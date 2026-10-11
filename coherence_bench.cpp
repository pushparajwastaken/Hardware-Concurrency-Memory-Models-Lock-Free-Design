// coherence_bench.cpp  -- benchmark driver for coherence_verify.cpp
// Build: g++ -std=c++17 -O2 coherence_bench.cpp -o coherence_bench
// Run  : ./coherence_bench > bench_results.csv   (progress/errors go to stderr)
//
// Every workload is executed through the checking Harness, so every number below
// comes from a run in which all invariants I1..I5 held (any violation aborts).
#define COHERENCE_NO_MAIN
#include "coherence_verify.cpp"

#include <chrono>
#include <cstdlib>

using Workload = std::function<void(Harness&, std::mt19937&, int /*cores*/, int /*ops*/)>;

static void producerConsumer(Harness& h, std::mt19937&, int n, int ops) {
    uint32_t v = 1;
    for (int done = 0; done < ops;) {
        h.write(0, 0x40, v++); done++;
        for (int c = 1; c < n && done < ops; ++c, ++done) h.read(c, 0x40);
    }
}
static void migratory(Harness& h, std::mt19937&, int n, int ops) {
    uint32_t v = 1;
    for (int i = 0; i < ops; i += 2) { int c = (i / 2) % n; h.read(c, 0x80); h.write(c, 0x80, v++); }
}
static void readMostly(Harness& h, std::mt19937& r, int n, int ops) {
    uint32_t v = 1;
    for (int i = 0; i < ops; ++i) {
        int c = r() % n; Addr a = (r() % 16) * 64;
        if (r() % 100 < 95) h.read(c, a); else h.write(c, a, v++);
    }
}
static void writeContention(Harness& h, std::mt19937& r, int n, int ops) {
    uint32_t v = 1;
    for (int i = 0; i < ops; ++i) h.write(r() % n, 0xC0, v++);
}
static void privateData(Harness& h, std::mt19937& r, int n, int ops) {
    uint32_t v = 1;
    for (int i = 0; i < ops; ++i) {
        int c = r() % n; Addr a = ((Addr)c * 8 + r() % 8) * 64;
        if (r() % 2) h.read(c, a); else h.write(c, a, v++);
    }
}
static void mixedRandom(Harness& h, std::mt19937& r, int n, int ops) {
    uint32_t v = 1;
    for (int i = 0; i < ops; ++i) {
        int c = r() % n; Addr a = (r() % 8) * 64; int k = r() % 100;
        if (k < 45) h.read(c, a); else if (k < 85) h.write(c, a, v++); else h.evict(c, a);
    }
}

static Stats runOne(const Workload& w, Protocol p, int cores, int ops, unsigned seed) {
    Harness h(cores, p);
    std::mt19937 rng(seed);
    w(h, rng, cores, ops);
    if (!h.ok()) { std::fprintf(stderr, "INVARIANT VIOLATION: %s\n", h.firstViolation.c_str()); std::exit(2); }
    return h.sys.stats();
}

static void row(const char* wl, Protocol p, int cores, int ops, const Stats& s) {
    std::printf("%s,%s,%d,%d,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n", wl, name(p), cores, ops,
                (unsigned long long)s.hits, (unsigned long long)s.misses, (unsigned long long)s.busRd,
                (unsigned long long)s.busRdX, (unsigned long long)s.busUpgr,
                (unsigned long long)s.memWrites, (unsigned long long)s.cacheToCache,
                (unsigned long long)s.silentEtoM);
}

int main() {
    const int OPS = 200000;
    std::printf("workload,protocol,cores,ops,hits,misses,busRd,busRdX,busUpgr,memWrites,cacheToCache,silentEtoM\n");

    std::vector<std::pair<const char*, Workload>> wls = {
        {"producer_consumer", producerConsumer}, {"migratory", migratory},
        {"read_mostly", readMostly},             {"write_contention", writeContention},
        {"private_data", privateData},           {"mixed_random", mixedRandom}};
    for (auto& [nm, w] : wls)
        for (Protocol p : {Protocol::MESI, Protocol::MOESI})
            row(nm, p, 8, OPS, runOne(w, p, 8, OPS, 1));

    // Core-count scaling (rows tagged with workload "scale_<name>")
    for (int n : {2, 4, 8, 16, 32})
        for (Protocol p : {Protocol::MESI, Protocol::MOESI}) {
            row("scale_producer_consumer", p, n, OPS, runOne(producerConsumer, p, n, OPS, 1));
            row("scale_mixed_random",      p, n, OPS, runOne(mixedRandom,      p, n, OPS, 1));
        }

    // Simulator throughput: raw System vs System + full invariant checking
    using clk = std::chrono::steady_clock;
    const int N = 2000000;
    for (Protocol p : {Protocol::MESI, Protocol::MOESI}) {
        { // raw
            System s(8, p); std::mt19937 r(3); uint32_t v = 1;
            auto t0 = clk::now();
            for (int i = 0; i < N; ++i) {
                int c = r() % 8; Addr a = (r() % 8) * 64; int k = r() % 100;
                if (k < 45) s.read(c, a); else if (k < 85) s.write(c, a, v++); else s.evict(c, a);
            }
            double sec = std::chrono::duration<double>(clk::now() - t0).count();
            std::fprintf(stderr, "throughput,%s,raw,%.3f\n", name(p), N / sec / 1e6);
            std::printf("throughput_raw,%s,8,%d,%.0f,0,0,0,0,0,0,0\n", name(p), N, N / sec);
        }
        { // checked
            Harness h(8, p); std::mt19937 r(3); uint32_t v = 1;
            auto t0 = clk::now();
            for (int i = 0; i < N; ++i) {
                int c = r() % 8; Addr a = (r() % 8) * 64; int k = r() % 100;
                if (k < 45) h.read(c, a); else if (k < 85) h.write(c, a, v++); else h.evict(c, a);
            }
            double sec = std::chrono::duration<double>(clk::now() - t0).count();
            if (!h.ok()) return 2;
            std::fprintf(stderr, "throughput,%s,checked,%.3f\n", name(p), N / sec / 1e6);
            std::printf("throughput_checked,%s,8,%d,%.0f,0,0,0,0,0,0,0\n", name(p), N, N / sec);
        }
    }
    return 0;
}
