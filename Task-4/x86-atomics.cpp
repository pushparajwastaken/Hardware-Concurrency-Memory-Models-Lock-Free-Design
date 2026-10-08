// Demonstration source for the supplied compiler/assembly comparison.
// Tested with: GCC 14.2.0, g++ -std=c++20 -O2 -S -masm=intel
// Target: compiler default x86-64 Linux target (not an explicit -march setting).
#include <atomic>

std::atomic<int> x{0};

int load_relaxed() { return x.load(std::memory_order_relaxed); }
int load_acquire() { return x.load(std::memory_order_acquire); }
void store_relaxed(int v) { x.store(v, std::memory_order_relaxed); }
void store_release(int v) { x.store(v, std::memory_order_release); }
void store_sc(int v) { x.store(v, std::memory_order_seq_cst); }
int fetch_add_relaxed(int v) { return x.fetch_add(v, std::memory_order_relaxed); }
int fetch_add_sc(int v) { return x.fetch_add(v, std::memory_order_seq_cst); }
void fence_sc() { std::atomic_thread_fence(std::memory_order_seq_cst); }
