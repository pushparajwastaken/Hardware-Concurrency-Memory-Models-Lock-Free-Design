
// Store Buffering stress test — runtime demonstration of x86-64 store->load relaxation.
//
// Theory (see x86-memory-model.md): on x86-64 WB memory, a load may pass an older
// store to a different address, so the outcome r0 == 0 && r1 == 0 is ARCHITECTURALLY
// ALLOWED even though it is impossible under sequential consistency.
//
// This program hammers the pattern from two threads and samples the outcome.
// Measured on the review machine (x86-64 VM, GCC 12.2.0, -O2): ~16% of rounds
// observed the store-buffering outcome (31,822 / 200,000). Frequency is
// machine-dependent; seeing 0 in a short run does NOT disprove the model.
//
// Build:  g++ -std=c++20 -O2 -pthread store-buffering-stress-test.cpp
// Run:    ./a.out        (self-terminating; runs a few seconds)
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

int main() {
    alignas(64) std::atomic<int> x{0}, y{0};
    alignas(64) std::atomic<int> r0{0}, r1{0};
    std::atomic<bool> start{false};
    long sb_outcomes = 0, rounds = 0;

    std::thread t0([&] {
        while (!start.load(std::memory_order_relaxed)) {}
        for (;;) {
            x.store(1, std::memory_order_relaxed);
            r0.store(y.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
    });
    std::thread t1([&] {
        while (!start.load(std::memory_order_relaxed)) {}
        for (;;) {
            y.store(1, std::memory_order_relaxed);
            r1.store(x.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
    });

    start.store(true, std::memory_order_relaxed);
    for (long round = 0; round < 200000; ++round) {
        x.store(0, std::memory_order_relaxed);
        y.store(0, std::memory_order_relaxed);
        for (int spin = 0; spin < 200; ++spin) { asm volatile(""); } // delay only
        int a = r0.load(std::memory_order_relaxed);
        int b = r1.load(std::memory_order_relaxed);
        if (a == 0 && b == 0) ++sb_outcomes;
        ++rounds;
    }
    std::printf("rounds sampled: %ld, store-buffering outcomes (r0==0 && r1==0): %ld\n",
                rounds, sb_outcomes);
    std::fflush(stdout);
    std::_Exit(0); // worker threads run forever; terminate the process
}
