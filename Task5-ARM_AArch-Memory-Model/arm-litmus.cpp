// arm-litmus.cpp  --  observe weak memory ordering on real hardware
//
// Build (Termux / Android, or any Linux/ARM box):
//   clang++ -O2 -std=c++17 -pthread arm-litmus.cpp -o litmus      (or g++)
//
// Run:   ./litmus [iterations] [CPU pairs ...] [output_dir]
//   ./litmus                           # 1,000,000 iterations, one unpinned run -> ./results
//   ./litmus 2000000 0:1 2:5 6:7       # run every test once per pair (T0 on first CPU, T1 on second)
//   ./litmus 2000000 auto              # pick 3 pairs from CPU max frequencies (see below)
//   ./litmus 2000000 auto run2         # same, but write into ./run2
//
// "auto" reads /sys/devices/system/cpu/cpuN/cpufreq/cpuinfo_max_freq and picks
//   (1) the two slowest CPUs, (2) the two fastest CPUs, (3) slowest + fastest (cross-cluster).
// If the frequencies are unreadable or all equal it falls back to (0,1), (n-2,n-1), (0,n-1).
//
// Output (one folder per CPU pair, one sub-folder per test):
//   results/summary.txt                           one line per (pair, test) incl. outcome counts
//   results/cpu0-1/SB_relaxed/reorders_vs_runs.txt     "runs cumulative_reorders"
//   results/cpu0-1/SB_relaxed/reorders_per_second.txt  "second reorders runs_in_that_second"
// Plot everything with:  python plot_litmus.py results
//
// Tests (the "weak" outcome is the one a sequentially consistent machine can never show):
//   SB  store buffering : T0: x=1; r0=y     T1: y=1; r1=x      weak: r0==0 && r1==0
//   MP  message passing : T0: data=1; flag=1  T1: r0=flag; r1=data   weak: r0==1 && r1==0
//   LB  load buffering  : T0: r0=x; y=1     T1: r1=y; x=1      weak: r0==1 && r1==1
//
// Each test is run with different memory orders / fences, so you can see which
// ones stop the weak outcome. Compare with arm-atomics.cpp to see the instructions.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include <utility>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <string>
#include <algorithm>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__aarch64__) || defined(__arm__)
  #define CPU_RELAX() asm volatile("yield")
#elif defined(__x86_64__) || defined(__i386__)
  #define CPU_RELAX() asm volatile("pause")
#else
  #define CPU_RELAX() do {} while (0)
#endif
#define COMPILER_BARRIER() asm volatile("" ::: "memory")   // stops the COMPILER only

using std::atomic;
using std::memory_order;
constexpr memory_order RLX = std::memory_order_relaxed;
constexpr memory_order ACQ = std::memory_order_acquire;
constexpr memory_order REL = std::memory_order_release;
constexpr memory_order SC  = std::memory_order_seq_cst;

// ----- shared state for one iteration (one cache line per variable) -----
struct alignas(64) Var { atomic<int> v{0}; };
struct Cell {
    Var x, y;
    alignas(64) int r0 = -1;
    alignas(64) int r1 = -1;
};
constexpr int NCELLS = 1024;
static Cell cells[NCELLS];

// ----- a test = two thread bodies + a predicate for the weak outcome -----
struct Test {
    const char *name;
    const char *what;
    void (*t0)(Cell &);
    void (*t1)(Cell &);
    bool (*weak)(int, int);
};

// ---------- SB: store buffering ----------
template <memory_order ST, memory_order LD, bool FENCE>
static void sb0(Cell &c) {
    c.x.v.store(1, ST);
    if (FENCE) std::atomic_thread_fence(SC); else COMPILER_BARRIER();
    c.r0 = c.y.v.load(LD);
}
template <memory_order ST, memory_order LD, bool FENCE>
static void sb1(Cell &c) {
    c.y.v.store(1, ST);
    if (FENCE) std::atomic_thread_fence(SC); else COMPILER_BARRIER();
    c.r1 = c.x.v.load(LD);
}
static bool sbWeak(int a, int b) { return a == 0 && b == 0; }

// ---------- MP: message passing (x = data, y = flag) ----------
template <memory_order FLAG_ST, bool FENCE>
static void mp0(Cell &c) {                       // writer
    c.x.v.store(1, RLX);
    if (FENCE) std::atomic_thread_fence(REL); else COMPILER_BARRIER();
    c.y.v.store(1, FLAG_ST);
}
template <memory_order FLAG_LD, bool FENCE>
static void mp1(Cell &c) {                       // reader
    c.r0 = c.y.v.load(FLAG_LD);
    if (FENCE) std::atomic_thread_fence(ACQ); else COMPILER_BARRIER();
    c.r1 = c.x.v.load(RLX);
}
static bool mpWeak(int a, int b) { return a == 1 && b == 0; }

// ---------- LB: load buffering ----------
template <memory_order LD, memory_order ST>
static void lb0(Cell &c) {
    c.r0 = c.x.v.load(LD);
    COMPILER_BARRIER();
    c.y.v.store(1, ST);
}
template <memory_order LD, memory_order ST>
static void lb1(Cell &c) {
    c.r1 = c.y.v.load(LD);
    COMPILER_BARRIER();
    c.x.v.store(1, ST);
}
static bool lbWeak(int a, int b) { return a == 1 && b == 1; }

static std::vector<Test> allTests() {
    return {
        {"SB relaxed",              "str; ldr",                 sb0<RLX,RLX,false>, sb1<RLX,RLX,false>, sbWeak},
        {"SB release/acquire",      "stlr; ldar",               sb0<REL,ACQ,false>, sb1<REL,ACQ,false>, sbWeak},
        {"SB seq_cst",              "stlr; ldar",               sb0<SC, SC, false>, sb1<SC, SC, false>, sbWeak},
        {"SB relaxed + dmb ish",    "str; dmb ish; ldr",        sb0<RLX,RLX,true >, sb1<RLX,RLX,true >, sbWeak},

        {"MP relaxed",              "str; str  /  ldr; ldr",    mp0<RLX,false>,     mp1<RLX,false>,     mpWeak},
        {"MP release/acquire",      "str; stlr  /  ldar; ldr",  mp0<REL,false>,     mp1<ACQ,false>,     mpWeak},
        {"MP relaxed + fences",     "str; dmb; str  /  ldr; dmb ishld; ldr", mp0<RLX,true>, mp1<RLX,true>, mpWeak},
        {"MP only writer release",  "str; stlr  /  ldr; ldr (reader weak)",  mp0<REL,false>, mp1<RLX,false>, mpWeak},

        {"LB relaxed",              "ldr; str",                 lb0<RLX,RLX>,       lb1<RLX,RLX>,       lbWeak},
        {"LB store-release",        "ldr; stlr",                lb0<RLX,REL>,       lb1<RLX,REL>,       lbWeak},
    };
}

// ----- two-thread start/finish synchronisation (spin barrier) -----
// Each iteration has two barriers (start, end) and each is passed by 2 threads,
// so iteration i completes its start barrier at count 4*i+2 and its end barrier at 4*i+4.
static atomic<long> barrierCount{0};
static void barrier(long target) {
    barrierCount.fetch_add(1, SC);
    int spins = 0;
    while (barrierCount.load(ACQ) < target) {
        CPU_RELAX();
        if (++spins > 20000) { sched_yield(); spins = 0; }   // be nice on 1-2 core machines
    }
}

// Random start offset: without it the thread that arrives second always runs first and
// the two bodies rarely overlap. Randomising the offset scans through all alignments.
constexpr unsigned DELAY_MAX = 128;                          // in nop-loop steps (~ns)
static inline unsigned xorshift(unsigned &s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
static inline void delaySteps(unsigned n) { while (n--) asm volatile("nop"); }

static void pinThisThread(int cpu) {
    if (cpu < 0) return;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {      // 0 = calling thread on Linux/Android
        static std::set<int> warned;                         // only warn once per CPU
        if (warned.insert(cpu).second)
            fprintf(stderr, "Warning: could not pin to CPU %d (Android may block this)\n", cpu);
    }
}

constexpr long SAMPLE_INTERVAL = 1000;   // log cumulative weak count every N iterations
constexpr long TIME_CHECK_EVERY = 256;   // read the clock every N iterations (keeps timing cheap)

static double nowSec() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

struct Result {
    long weak = 0;
    long other = 0;
    long count[2][2] = {{0, 0}, {0, 0}};
    double runtime = 0;
    std::vector<long> sampleRuns, sampleCum;       // graph 1 data
    std::vector<long> perSecWeak, perSecRuns;      // graph 2 data
};

static Result runTest(const Test &t, long iters, int cpu0, int cpu1) {
    for (auto &c : cells) { c.x.v.store(0, RLX); c.y.v.store(0, RLX); c.r0 = c.r1 = -1; }
    barrierCount.store(0);
    Result res;

    std::thread worker([&] {                                   // runs T1
        pinThisThread(cpu1);
        unsigned seed = 0x1234567u;
        for (long i = 0; i < iters; i++) {
            Cell &c = cells[i % NCELLS];
            barrier(4 * i + 2);
            delaySteps(xorshift(seed) % DELAY_MAX);
            t.t1(c);
            barrier(4 * i + 4);
        }
    });

    pinThisThread(cpu0);                                       // this thread runs T0 and records
    unsigned seed = 0x9e3779b9u;
    double t0 = nowSec();
    size_t curSec = 0;
    res.perSecWeak.push_back(0);
    res.perSecRuns.push_back(0);
    for (long i = 0; i < iters; i++) {
        Cell &c = cells[i % NCELLS];
        barrier(4 * i + 2);
        delaySteps(xorshift(seed) % DELAY_MAX);
        t.t0(c);
        barrier(4 * i + 4);

        int a = c.r0, b = c.r1;                                // both threads are finished with this cell
        if ((a == 0 || a == 1) && (b == 0 || b == 1)) res.count[a][b]++; else res.other++;
        bool weak = t.weak(a, b);
        if (weak) res.weak++;
        c.x.v.store(0, RLX); c.y.v.store(0, RLX); c.r0 = c.r1 = -1;   // reset for reuse

        // ---- data logging (in memory only; written to disk after the test) ----
        if (i % TIME_CHECK_EVERY == 0) {
            curSec = (size_t)(nowSec() - t0);
            while (res.perSecRuns.size() <= curSec) { res.perSecWeak.push_back(0); res.perSecRuns.push_back(0); }
        }
        res.perSecRuns[curSec]++;
        if (weak) res.perSecWeak[curSec]++;
        if ((i + 1) % SAMPLE_INTERVAL == 0) {
            res.sampleRuns.push_back(i + 1);
            res.sampleCum.push_back(res.weak);
        }
    }
    res.runtime = nowSec() - t0;
    worker.join();
    return res;
}

// "SB relaxed + dmb ish" -> "SB_relaxed_dmb_ish"
static std::string slug(const char *name) {
    std::string s;
    for (const char *p = name; *p; p++) {
        char ch = *p;
        bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
        if (ok) s += ch;
        else if (!s.empty() && s.back() != '_') s += '_';
    }
    while (!s.empty() && s.back() == '_') s.pop_back();
    return s;
}

static void makeDir(const std::string &path) {
    if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST)
        fprintf(stderr, "Warning: could not create directory %s\n", path.c_str());
}

// ----- CPU pairs -----
struct CpuPair { int a, b; std::string label; };

static CpuPair makePair(int a, int b) {
    CpuPair p;
    p.a = a; p.b = b;
    p.label = a < 0 ? "unpinned" : "cpu" + std::to_string(a) + "-" + std::to_string(b);
    return p;
}

// Max frequency (kHz) of every CPU; 0 = unknown.
static std::vector<long> readMaxFreqs() {
    std::vector<long> f;
    if (const char *fake = getenv("LITMUS_FAKE_FREQS")) {      // testing hook, e.g. "1800,1800,2400,2800"
        const char *p = fake;
        while (*p) { f.push_back(atol(p)); while (*p && *p != ',') p++; if (*p == ',') p++; }
        return f;
    }
    unsigned n = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < n; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_max_freq", i);
        long v = 0;
        FILE *fp = fopen(path, "r");
        if (fp) { if (fscanf(fp, "%ld", &v) != 1) v = 0; fclose(fp); }
        f.push_back(v);
    }
    return f;
}

static std::vector<CpuPair> autoPairs(const std::vector<long> &f) {
    int n = (int)f.size();
    std::vector<CpuPair> out;
    if (n < 2) return out;
    bool known = true;
    long mn = f[0], mx = f[0];
    for (long v : f) { if (v <= 0) known = false; mn = std::min(mn, v); mx = std::max(mx, v); }

    std::vector<std::pair<int,int>> cand;
    if (known && mn < mx) {
        std::vector<int> order(n);
        for (int i = 0; i < n; i++) order[i] = i;
        std::stable_sort(order.begin(), order.end(), [&](int x, int y) { return f[x] < f[y]; });
        cand = {{order[0], order[1]}, {order[n - 2], order[n - 1]}, {order[0], order[n - 1]}};
    } else {
        cand = {{0, 1}, {n - 2, n - 1}, {0, n - 1}};
    }
    std::set<std::pair<int,int>> seen;
    for (auto &c : cand)
        if (seen.insert(c).second) out.push_back(makePair(c.first, c.second));
    return out;
}

// Writes the two files in exactly the format plot_reorders.py / plot_litmus.py read.
static void writeResultFiles(const std::string &outDir, const CpuPair &pair, const Test &t,
                             const Result &r, long iters) {
    std::string pairDir = outDir + "/" + pair.label;
    makeDir(pairDir);
    std::string dir = pairDir + "/" + slug(t.name);
    makeDir(dir);

    FILE *f1 = fopen((dir + "/reorders_vs_runs.txt").c_str(), "w");
    if (f1) {
        fprintf(f1, "# runs cumulative_reorders\n");
        for (size_t i = 0; i < r.sampleRuns.size(); i++)
            fprintf(f1, "%ld %ld\n", r.sampleRuns[i], r.sampleCum[i]);
        fclose(f1);
    }

    FILE *f2 = fopen((dir + "/reorders_per_second.txt").c_str(), "w");
    if (f2) {
        fprintf(f2, "# test %s total_runtime_sec %.3f total_runs %ld total_reorders %ld cpus %d,%d\n",
                t.name, r.runtime, iters, r.weak, pair.a, pair.b);
        fprintf(f2, "# second reorders runs_in_that_second\n");
        for (size_t i = 0; i < r.perSecRuns.size(); i++)
            fprintf(f2, "%zu %ld %ld\n", i + 1, r.perSecWeak[i], r.perSecRuns[i]);
        fclose(f2);
    }
}

int main(int argc, char **argv) {
    long iters = argc > 1 ? atol(argv[1]) : 1000000;
    std::vector<CpuPair> pairs;
    std::string outDir = "results";
    bool wantAuto = false;

    for (int i = 2; i < argc; i++) {
        std::string tok = argv[i];
        if (tok == "auto") wantAuto = true;
        else if (tok == "none") pairs.push_back(makePair(-1, -1));
        else if (tok.find(':') != std::string::npos) {
            int a = -1, b = -1;
            if (sscanf(tok.c_str(), "%d:%d", &a, &b) == 2 && a >= 0 && b >= 0)
                pairs.push_back(makePair(a, b));
            else { fprintf(stderr, "Bad CPU pair '%s' (expected A:B, e.g. 0:3)\n", tok.c_str()); return 1; }
        }
        else outDir = tok;
    }
    if (wantAuto) {
        std::vector<long> f = readMaxFreqs();
        std::vector<CpuPair> ap = autoPairs(f);
        printf("auto: max frequencies (kHz):");
        for (size_t i = 0; i < f.size(); i++) printf(" cpu%zu=%ld", i, f[i]);
        printf("\n");
        for (auto &p : ap) pairs.push_back(p);
    }
    if (pairs.empty()) pairs.push_back(makePair(-1, -1));
    makeDir(outDir);

#if defined(__aarch64__)
    const char *arch = "AArch64";
#elif defined(__x86_64__)
    const char *arch = "x86-64";
#else
    const char *arch = "other";
#endif
    printf("Architecture: %s, hardware threads: %u, iterations per test: %ld\n",
           arch, std::thread::hardware_concurrency(), iters);
    printf("CPU pairs:");
    for (auto &p : pairs) printf(" %s", p.label.c_str());
    printf("\nResults folder: %s/\n", outDir.c_str());
    printf("Outcome columns are (r0,r1). The weak outcome is marked with '*'.\n\n");

    FILE *sum = fopen((outDir + "/summary.txt").c_str(), "w");
    if (sum) fprintf(sum, "# pair test iterations weak_count weak_percent runtime_sec c00 c01 c10 c11\n");

    for (const CpuPair &pair : pairs) {
        if (pair.a >= 0) printf("==================== %s  (T0 -> CPU %d, T1 -> CPU %d) ====================\n\n",
                                pair.label.c_str(), pair.a, pair.b);
        else printf("==================== unpinned ====================\n\n");
        for (const Test &t : allTests()) {
            Result r = runTest(t, iters, pair.a, pair.b);
            printf("%-24s [%s]\n", t.name, t.what);
            printf("   ");
            for (int a = 0; a < 2; a++)
                for (int b = 0; b < 2; b++)
                    printf("(%d,%d)%s=%-9ld ", a, b, t.weak(a, b) ? "*" : " ", r.count[a][b]);
            printf("\n   weak outcome: %ld of %ld (%.4f%%)  -> %s\n\n", r.weak, iters,
                   100.0 * r.weak / iters, r.weak ? "OBSERVED" : "not observed");

            writeResultFiles(outDir, pair, t, r, iters);
            if (sum) {
                fprintf(sum, "%s %s %ld %ld %.4f %.3f %ld %ld %ld %ld\n", pair.label.c_str(),
                        slug(t.name).c_str(), iters, r.weak, 100.0 * r.weak / iters, r.runtime,
                        r.count[0][0], r.count[0][1], r.count[1][0], r.count[1][1]);
                fflush(sum);
            }
        }
    }
    if (sum) fclose(sum);
    printf("Done. Data written under %s/<pair>/<test>/ and %s/summary.txt\n"
           "Plot with: python plot_litmus.py %s\n", outDir.c_str(), outDir.c_str(), outDir.c_str());
    return 0;
}