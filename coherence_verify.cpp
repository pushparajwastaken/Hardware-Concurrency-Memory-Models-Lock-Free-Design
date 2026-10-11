// coherence_verify.cpp
// -----------------------------------------------------------------------------
// MESI / MOESI snooping cache-coherence model + verifier.
//
//   * System      : N private caches + shared memory, atomic snooping bus.
//   * Checker     : protocol invariants checked after EVERY operation:
//        I1  SWMR       : if a line is M or E in one cache, no other cache holds it.
//        I2  Ownership  : at most one M/O/E ("owner") copy; MESI has no O state.
//        I3  Data value : all valid copies hold identical data.
//        I4  Memory     : if no dirty copy (M/O) exists, valid copies == memory.
//        I5  Golden     : every load returns the value of the latest store
//                         (sequential reference model).
//   * Tests       : directed transition tests, randomized stress tests, and
//                   fault-injection tests (deliberately buggy protocols must be
//                   caught by the checker -> proves the checker has teeth).
//
// Build : g++ -std=c++17 -O2 -Wall -Wextra coherence_verify.cpp -o coherence_verify
// Run   : ./coherence_verify
// -----------------------------------------------------------------------------
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using Addr = uint64_t;

enum class State { I, S, E, O, M };
enum class Protocol { MESI, MOESI };
// Injected faults (used only by the fault-injection tests)
enum class Bug {
    None,
    SkipInvalidateOnUpgrade,  // S/O -> M write does not invalidate other sharers
    NoDowngradeEOnRead,       // E copy stays E when another cache reads the line
    NoFlushOnMtoS,            // (MESI) M -> S on remote read without memory update
    LostWritebackOnEvict,     // evicting M/O line does not write back
    WriteMissNoInvalidate     // BusRdX does not invalidate other copies
};

static const char* name(State s) {
    switch (s) {
        case State::I: return "I"; case State::S: return "S";
        case State::E: return "E"; case State::O: return "O";
        default:       return "M";
    }
}
static const char* name(Protocol p) { return p == Protocol::MESI ? "MESI" : "MOESI"; }
static const char* name(Bug b) {
    switch (b) {
        case Bug::None: return "None";
        case Bug::SkipInvalidateOnUpgrade: return "SkipInvalidateOnUpgrade";
        case Bug::NoDowngradeEOnRead: return "NoDowngradeEOnRead";
        case Bug::NoFlushOnMtoS: return "NoFlushOnMtoS";
        case Bug::LostWritebackOnEvict: return "LostWritebackOnEvict";
        default: return "WriteMissNoInvalidate";
    }
}

struct Line { State st = State::I; uint32_t data = 0; };

struct Stats {
    uint64_t hits = 0, misses = 0, busRd = 0, busRdX = 0, busUpgr = 0,
             memWrites = 0, cacheToCache = 0, silentEtoM = 0;
};

// ----------------------------------------------------------------------------
// Coherent memory system (one 32-bit word per line; infinite-capacity caches,
// lines leave only through explicit evict()).
// ----------------------------------------------------------------------------
class System {
public:
    System(int n, Protocol p, Bug b = Bug::None) : n_(n), proto_(p), bug_(b), cache_(n) {}

    int cores() const { return n_; }
    Protocol protocol() const { return proto_; }
    const Stats& stats() const { return st_; }

    State state(int c, Addr a) const {
        auto it = cache_[c].find(a);
        return it == cache_[c].end() ? State::I : it->second.st;
    }
    uint32_t cachedData(int c, Addr a) const {
        auto it = cache_[c].find(a);
        return it == cache_[c].end() ? 0 : it->second.data;
    }
    uint32_t memData(Addr a) const {
        auto it = mem_.find(a);
        return it == mem_.end() ? 0 : it->second;
    }

    // ---- processor-side operations -----------------------------------------
    uint32_t read(int c, Addr a) {
        Line& l = cache_[c][a];
        if (l.st != State::I) { st_.hits++; return l.data; }  // hit in S/E/O/M
        st_.misses++; st_.busRd++;
        bool others = false;
        bool haveSupplier = false;
        uint32_t data = memData(a);
        for (int o = 0; o < n_; ++o) {
            if (o == c) continue;
            Line& r = cache_[o][a];
            switch (r.st) {
                case State::M:
                    others = true; haveSupplier = true; data = r.data; st_.cacheToCache++;
                    if (proto_ == Protocol::MOESI) {
                        r.st = State::O;                       // keep dirty ownership
                    } else {
                        if (bug_ != Bug::NoFlushOnMtoS) { mem_[a] = r.data; st_.memWrites++; }
                        r.st = State::S;                       // MESI: flush + share
                    }
                    break;
                case State::O:
                    others = true; haveSupplier = true; data = r.data; st_.cacheToCache++;
                    break;                                     // stays Owned
                case State::E:
                    others = true; haveSupplier = true; data = r.data; st_.cacheToCache++;
                    if (bug_ != Bug::NoDowngradeEOnRead) r.st = State::S;
                    break;
                case State::S:
                    others = true;
                    break;
                case State::I: break;
            }
        }
        (void)haveSupplier;
        l.data = data;
        l.st = others ? State::S : State::E;
        return l.data;
    }

    void write(int c, Addr a, uint32_t v) {
        Line& l = cache_[c][a];
        switch (l.st) {
            case State::M:
                st_.hits++; break;
            case State::E:
                st_.hits++; st_.silentEtoM++; break;           // silent upgrade, no bus
            case State::S:
            case State::O:
                st_.hits++; st_.busUpgr++;
                if (bug_ != Bug::SkipInvalidateOnUpgrade) invalidateOthers(c, a);
                break;
            case State::I:
                st_.misses++; st_.busRdX++;
                if (bug_ != Bug::WriteMissNoInvalidate) invalidateOthers(c, a);
                break;
        }
        l.st = State::M;
        l.data = v;
    }

    // Capacity/conflict eviction of a line from cache c.
    void evict(int c, Addr a) {
        auto it = cache_[c].find(a);
        if (it == cache_[c].end()) return;
        Line& l = it->second;
        if ((l.st == State::M || l.st == State::O) && bug_ != Bug::LostWritebackOnEvict) {
            mem_[a] = l.data; st_.memWrites++;
        }
        cache_[c].erase(it);
    }

    // ---- invariant checker: returns "" if OK, else a description -----------
    std::string check(Addr a) const {
        int M = 0, O = 0, E = 0, S = 0, valid = 0;
        bool haveData = false; uint32_t ref = 0;
        std::ostringstream err;
        for (int c = 0; c < n_; ++c) {
            State s = state(c, a);
            if (s == State::I) continue;
            valid++;
            M += s == State::M; O += s == State::O; E += s == State::E; S += s == State::S;
            if (!haveData) { ref = cachedData(c, a); haveData = true; }
            else if (cachedData(c, a) != ref) {
                err << "I3 violated: valid copies disagree on data. ";
            }
        }
        if ((M || E) && valid != 1) err << "I1 (SWMR) violated: M/E coexists with other copies. ";
        if (M + O + E > 1)          err << "I2 violated: more than one owner. ";
        if (proto_ == Protocol::MESI && O) err << "I2 violated: Owned state in MESI. ";
        if (haveData && !(M || O) && ref != memData(a))
            err << "I4 violated: clean copies differ from memory. ";
        std::string e = err.str();
        if (!e.empty()) e += "[" + dump(a) + "]";
        return e;
    }

    std::string dump(Addr a) const {
        std::ostringstream o;
        o << "addr=" << a << " states=";
        for (int c = 0; c < n_; ++c) o << name(state(c, a));
        o << " mem=" << memData(a);
        return o.str();
    }

private:
    void invalidateOthers(int c, Addr a) {
        for (int o = 0; o < n_; ++o) {
            if (o == c) continue;
            auto it = cache_[o].find(a);
            if (it != cache_[o].end()) cache_[o].erase(it);   // M/O data is dropped:
        }                                                      // writer overwrites the whole line
    }

    int n_;
    Protocol proto_;
    Bug bug_;
    std::vector<std::unordered_map<Addr, Line>> cache_;
    std::unordered_map<Addr, uint32_t> mem_;
    Stats st_;
};

// ----------------------------------------------------------------------------
// Verification harness: wraps System with golden model + per-op invariant check.
// ----------------------------------------------------------------------------
class Harness {
public:
    Harness(int n, Protocol p, Bug b = Bug::None) : sys(n, p, b) {}
    System sys;
    std::string firstViolation;
    uint64_t ops = 0;

    uint32_t read(int c, Addr a) {
        uint32_t got = sys.read(c, a);
        uint32_t want = golden_.count(a) ? golden_[a] : 0;
        if (got != want) fail("I5 violated: load core" + std::to_string(c) + " got " +
                              std::to_string(got) + " expected " + std::to_string(want) +
                              " [" + sys.dump(a) + "]");
        after(a);
        return got;
    }
    void write(int c, Addr a, uint32_t v) { sys.write(c, a, v); golden_[a] = v; after(a); }
    void evict(int c, Addr a)             { sys.evict(c, a);               after(a); }
    bool ok() const { return firstViolation.empty(); }

private:
    void after(Addr a) {
        ops++;
        std::string e = sys.check(a);
        if (!e.empty()) fail(e);
    }
    void fail(const std::string& m) { if (firstViolation.empty()) firstViolation = m; }
    std::unordered_map<Addr, uint32_t> golden_;
};

// ----------------------------------------------------------------------------
// Tiny test framework
// ----------------------------------------------------------------------------
struct TestResult { std::string name; bool pass; std::string detail; };
static std::vector<TestResult> g_results;

static std::string states(const System& s, Addr a) {
    std::string r; for (int c = 0; c < s.cores(); ++c) r += name(s.state(c, a)); return r;
}

#define EXPECT_STATES(h, addr, expect)                                              \
    do { std::string got = states((h).sys, (addr));                                 \
         if (got != (expect)) { fail_ = "states " + got + " != expected " + (expect); return; } } while (0)
#define EXPECT_TRUE(cond, msg) do { if (!(cond)) { fail_ = (msg); return; } } while (0)

static void run(const std::string& n, const std::function<void(std::string&)>& body) {
    std::string fail;
    body(fail);
    g_results.push_back({n, fail.empty(), fail});
}
// Convenience: lambda body with 'fail_' alias
#define TEST(NAME, ...) run(NAME, [&](std::string& fail_) { __VA_ARGS__ })

// ----------------------------------------------------------------------------
// Directed tests: MESI
// ----------------------------------------------------------------------------
static void mesiDirected() {
    const Addr A = 0x40;
    TEST("MESI-01 cold read miss -> E", {
        Harness h(3, Protocol::MESI);
        h.read(0, A);
        EXPECT_STATES(h, A, "EII"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-02 E->M on write is silent (no bus traffic)", {
        Harness h(3, Protocol::MESI);
        h.read(0, A);
        auto before = h.sys.stats();
        h.write(0, A, 7);
        EXPECT_STATES(h, A, "MII");
        EXPECT_TRUE(h.sys.stats().busRdX == before.busRdX && h.sys.stats().busUpgr == before.busUpgr,
                    "bus transaction issued on E->M");
        EXPECT_TRUE(h.sys.stats().silentEtoM == 1, "silent upgrade not counted");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-03 second reader: E,I -> S,S", {
        Harness h(3, Protocol::MESI);
        h.read(0, A); h.read(1, A);
        EXPECT_STATES(h, A, "SSI"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-04 write to S invalidates other sharers (BusUpgr)", {
        Harness h(3, Protocol::MESI);
        h.read(0, A); h.read(1, A); h.read(2, A);
        h.write(1, A, 5);
        EXPECT_STATES(h, A, "IMI"); EXPECT_TRUE(h.sys.stats().busUpgr == 1, "no BusUpgr");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-05 remote read of M: M->S, flush to memory", {
        Harness h(3, Protocol::MESI);
        h.write(0, A, 42);
        EXPECT_STATES(h, A, "MII");
        EXPECT_TRUE(h.read(1, A) == 42, "reader got stale data");
        EXPECT_STATES(h, A, "SSI");
        EXPECT_TRUE(h.sys.memData(A) == 42, "memory not updated on M->S");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-06 write miss (BusRdX) on remote M: M->I, requester M", {
        Harness h(3, Protocol::MESI);
        h.write(0, A, 1); h.write(1, A, 2);
        EXPECT_STATES(h, A, "IMI");
        EXPECT_TRUE(h.read(2, A) == 2, "stale read");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-07 remote read of E: E->S", {
        Harness h(2, Protocol::MESI);
        h.read(0, A); h.read(1, A);
        EXPECT_STATES(h, A, "SS"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-08 evict M writes back; evict S/E silent", {
        Harness h(2, Protocol::MESI);
        h.write(0, A, 99); h.evict(0, A);
        EXPECT_STATES(h, A, "II"); EXPECT_TRUE(h.sys.memData(A) == 99, "no writeback");
        uint64_t w = h.sys.stats().memWrites;
        h.read(1, A); h.evict(1, A);
        EXPECT_TRUE(h.sys.stats().memWrites == w, "clean evict wrote memory");
        EXPECT_TRUE(h.read(0, A) == 99, "value lost after evict");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-09 MESI never produces Owned state", {
        Harness h(3, Protocol::MESI);
        h.write(0, A, 3); h.read(1, A); h.read(2, A);
        for (int c = 0; c < 3; ++c) EXPECT_TRUE(h.sys.state(c, A) != State::O, "O seen in MESI");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MESI-10 independent addresses do not interfere", {
        Harness h(2, Protocol::MESI);
        h.write(0, 0x00, 1); h.write(1, 0x40, 2);
        EXPECT_STATES(h, 0x00, "MI"); EXPECT_STATES(h, 0x40, "IM");
        EXPECT_TRUE(h.read(1, 0x00) == 1 && h.read(0, 0x40) == 2, "cross-addr data wrong");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
}

// ----------------------------------------------------------------------------
// Directed tests: MOESI
// ----------------------------------------------------------------------------
static void moesiDirected() {
    const Addr A = 0x80;
    TEST("MOESI-01 cold read -> E, write -> M silently", {
        Harness h(3, Protocol::MOESI);
        h.read(0, A); EXPECT_STATES(h, A, "EII");
        h.write(0, A, 1); EXPECT_STATES(h, A, "MII");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-02 remote read of M: M->O, memory stays STALE", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 11);
        EXPECT_TRUE(h.read(1, A) == 11, "wrong data from owner");
        EXPECT_STATES(h, A, "OSI");
        EXPECT_TRUE(h.sys.memData(A) == 0, "memory updated (should be deferred in MOESI)");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-03 further readers served by O; O persists", {
        Harness h(4, Protocol::MOESI);
        h.write(0, A, 5); h.read(1, A); h.read(2, A); h.read(3, A);
        EXPECT_STATES(h, A, "OSSS"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-04 write to O invalidates sharers -> M", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 5); h.read(1, A); h.read(2, A);
        h.write(0, A, 6);
        EXPECT_STATES(h, A, "MII"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-05 write to S while another cache is O -> O invalidated", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 5); h.read(1, A);
        h.write(1, A, 8);
        EXPECT_STATES(h, A, "IMI");
        EXPECT_TRUE(h.read(2, A) == 8, "stale read");
        EXPECT_STATES(h, A, "IOS");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-06 evict O writes back; sharers stay S and match memory", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 77); h.read(1, A); h.read(2, A);
        h.evict(0, A);
        EXPECT_STATES(h, A, "ISS");
        EXPECT_TRUE(h.sys.memData(A) == 77, "O writeback lost");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-07 evict sharer S while O exists: silent", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 9); h.read(1, A);
        uint64_t w = h.sys.stats().memWrites;
        h.evict(1, A);
        EXPECT_STATES(h, A, "OII"); EXPECT_TRUE(h.sys.stats().memWrites == w, "unexpected writeback");
        EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-08 write miss on O line: all copies invalidated", {
        Harness h(3, Protocol::MOESI);
        h.write(0, A, 1); h.read(1, A);
        h.write(2, A, 2);
        EXPECT_STATES(h, A, "IIM"); EXPECT_TRUE(h.ok(), h.firstViolation);
    });
    TEST("MOESI-09 MOESI issues fewer memory writes than MESI (producer/consumer)", {
        Harness m(2, Protocol::MESI), o(2, Protocol::MOESI);
        for (uint32_t i = 1; i <= 100; ++i) {
            m.write(0, A, i); m.read(1, A);
            o.write(0, A, i); o.read(1, A);
        }
        EXPECT_TRUE(m.sys.stats().memWrites > o.sys.stats().memWrites,
                    "MOESI did not reduce memory writes");
        EXPECT_TRUE(m.ok() && o.ok(), m.firstViolation + o.firstViolation);
    });
}

// ----------------------------------------------------------------------------
// Randomized stress test: returns first violation (empty = clean)
// ----------------------------------------------------------------------------
static std::string stress(Protocol p, Bug b, unsigned seed, int ncores, int naddrs, int nops,
                          uint64_t* opsDone = nullptr) {
    Harness h(ncores, p, b);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> core(0, ncores - 1), addr(0, naddrs - 1), kind(0, 99);
    uint32_t counter = 1;
    for (int i = 0; i < nops && h.ok(); ++i) {
        int c = core(rng); Addr a = (Addr)addr(rng) * 64; int k = kind(rng);
        if (k < 45)      h.read(c, a);
        else if (k < 85) h.write(c, a, counter++);
        else             h.evict(c, a);
    }
    if (opsDone) *opsDone = h.ops;
    return h.firstViolation;
}

static void stressTests() {
    for (Protocol p : {Protocol::MESI, Protocol::MOESI}) {
        for (unsigned seed : {1u, 2u, 3u, 42u, 2024u}) {
            std::string tn = std::string(name(p)) + "-RND seed=" + std::to_string(seed) +
                             " 4 cores x 6 addrs x 100k ops";
            TEST(tn, {
                uint64_t ops = 0;
                std::string v = stress(p, Bug::None, seed, 4, 6, 100000, &ops);
                if (!v.empty()) { fail_ = v; return; }
            });
        }
        TEST(std::string(name(p)) + "-RND heavy contention: 8 cores x 1 addr x 100k ops", {
            std::string v = stress(p, Bug::None, 7, 8, 1, 100000);
            if (!v.empty()) { fail_ = v; return; }
        });
        TEST(std::string(name(p)) + "-RND wide sharing: 16 cores x 32 addrs x 200k ops", {
            std::string v = stress(p, Bug::None, 99, 16, 32, 200000);
            if (!v.empty()) { fail_ = v; return; }
        });
    }
}

// ----------------------------------------------------------------------------
// Fault-injection: checker MUST flag each buggy protocol (pass = bug detected)
// ----------------------------------------------------------------------------
static void faultInjection() {
    struct Case { Protocol p; Bug b; };
    std::vector<Case> cases = {
        {Protocol::MESI,  Bug::SkipInvalidateOnUpgrade},
        {Protocol::MOESI, Bug::SkipInvalidateOnUpgrade},
        {Protocol::MESI,  Bug::NoDowngradeEOnRead},
        {Protocol::MOESI, Bug::NoDowngradeEOnRead},
        {Protocol::MESI,  Bug::NoFlushOnMtoS},
        {Protocol::MESI,  Bug::LostWritebackOnEvict},
        {Protocol::MOESI, Bug::LostWritebackOnEvict},
        {Protocol::MESI,  Bug::WriteMissNoInvalidate},
        {Protocol::MOESI, Bug::WriteMissNoInvalidate},
    };
    for (auto& c : cases) {
        std::string tn = std::string("FAULT ") + name(c.p) + " + " + name(c.b) + " is detected";
        run(tn, [&](std::string& fail_) {
            std::string v = stress(c.p, c.b, 5, 4, 4, 20000);
            if (v.empty()) fail_ = "checker MISSED the injected bug";
        });
    }
}

#ifndef COHERENCE_NO_MAIN
int main() {
    mesiDirected();
    moesiDirected();
    stressTests();
    faultInjection();

    int pass = 0;
    for (auto& r : g_results) {
        std::printf("[%s] %s%s%s\n", r.pass ? "PASS" : "FAIL", r.name.c_str(),
                    r.pass ? "" : "\n       -> ", r.pass ? "" : r.detail.c_str());
        pass += r.pass;
    }
    std::printf("\n%d / %zu tests passed\n", pass, g_results.size());
    return pass == (int)g_results.size() ? 0 : 1;
}
#endif
