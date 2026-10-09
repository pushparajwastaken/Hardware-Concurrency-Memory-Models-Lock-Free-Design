/*
 * Concept : Concurrency vs Parallelism and False Sharing (cache lines)
 * Overview: Compares one thread against many threads doing the same work, then
 *           benchmarks threads updating their own counters when the counters share
 *           one 64-byte cache line versus when each counter is padded to its own line.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

using Clock = steady_clock;

const size_t CACHE_LINE = 64;
const uint64_t ITERATIONS = 100000000;

struct Unpadded {
    atomic<uint64_t> value{0};
};

struct alignas(CACHE_LINE) Padded {
    atomic<uint64_t> value{0};
};

uint64_t heavyWork(uint64_t n) {
    uint64_t x = 1;
    for (uint64_t i = 0; i < n; ++i)
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
    return x;
}

void sequentialVsParallel() {
    unsigned threads = thread::hardware_concurrency();
    if (threads == 0) threads = 1;
    const uint64_t total = 400000000;

    auto t0 = Clock::now();
    volatile uint64_t single = heavyWork(total);
    (void)single;
    auto t1 = Clock::now();

    vector<thread> pool;
    vector<uint64_t> results(threads);
    for (unsigned i = 0; i < threads; ++i)
        pool.emplace_back([&, i] { results[i] = heavyWork(total / threads); });
    for (auto& t : pool) t.join();
    auto t2 = Clock::now();

    double seqMs = duration<double, milli>(t1 - t0).count();
    double parMs = duration<double, milli>(t2 - t1).count();

    cout << "[1] Sequential vs parallel (same total work)\n";
    cout << "    hardware threads : " << threads << "\n";
    cout << "    1 thread         : " << fixed << setprecision(1) << seqMs << " ms\n";
    cout << "    " << threads << " threads        : " << parMs << " ms\n";
    cout << "    speed-up         : " << setprecision(2) << seqMs / parMs << "x\n";
}

template <typename Counter>
double runCounters(unsigned threads) {
    vector<Counter> counters(threads);
    vector<thread> pool;

    auto start = Clock::now();
    for (unsigned i = 0; i < threads; ++i)
        pool.emplace_back([&counters, i] {
            for (uint64_t k = 0; k < ITERATIONS; ++k)
                counters[i].value.fetch_add(1, memory_order_relaxed);
        });
    for (auto& t : pool) t.join();
    auto end = Clock::now();

    return duration<double, milli>(end - start).count();
}

void falseSharingTest() {
    unsigned maxThreads = thread::hardware_concurrency();
    if (maxThreads == 0) maxThreads = 4;

    cout << "\n[2] False sharing benchmark (" << ITERATIONS << " increments per thread)\n";
    cout << "    sizeof(Unpadded) = " << sizeof(Unpadded) << " bytes, sizeof(Padded) = "
         << sizeof(Padded) << " bytes\n";
    cout << "    threads  adjacent(ms)  padded(ms)  slowdown\n";

    for (unsigned t = 1; t <= maxThreads && t <= 8; t *= 2) {
        double adjacent = runCounters<Unpadded>(t);
        double padded = runCounters<Padded>(t);
        cout << "    " << setw(7) << t << "  " << fixed << setprecision(1) << setw(12) << adjacent
             << "  " << setw(10) << padded << "  " << setprecision(2) << adjacent / padded << "x\n";
    }
}

int main() {
    sequentialVsParallel();
    falseSharingTest();
    return 0;
}
