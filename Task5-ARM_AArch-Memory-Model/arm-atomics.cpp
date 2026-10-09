// arm-atomics.cpp
// Tiny functions, one per atomic operation / memory order, so you can read
// the generated assembly. Nothing here is meant to be run.
//
//   g++     -O2 -S arm-atomics.cpp -o arm-atomics.s                      # baseline ARMv8.0
//   g++     -O2 -S -march=armv8.1-a arm-atomics.cpp -o arm-atomics-lse.s # with LSE atomics
//   clang++ -O2 -S arm-atomics.cpp -o arm-atomics.s                      # on Termux / Android
//
// (cross-compile from x86: use aarch64-linux-gnu-g++ instead of g++.)
// Note: GCC 10.1+ defaults to "outline atomics" on AArch64, so the baseline output calls
// helpers like __aarch64_ldadd4_acq_rel (they pick LSE or ldxr/stxr at runtime). Add
// -mno-outline-atomics to see the inline ldxr/stxr loops instead.
// Compare the outputs for the fetch_add / exchange / cas functions:
// ARMv8.0 uses ldxr/stxr retry loops, ARMv8.1 LSE uses single instructions
// (ldadd, swp, cas).
#include <atomic>
#include <stdint.h>
using std::atomic;
using std::memory_order_relaxed;
using std::memory_order_acquire;
using std::memory_order_release;
using std::memory_order_acq_rel;
using std::memory_order_seq_cst;

atomic<int> a;          // shared variable used by all functions

// ---------------- plain loads and stores ----------------
extern "C" int  load_relaxed()      { return a.load(memory_order_relaxed); }  // ldr
extern "C" int  load_acquire()      { return a.load(memory_order_acquire); }  // ldar
extern "C" int  load_seq_cst()      { return a.load(memory_order_seq_cst); }  // ldar
extern "C" void store_relaxed(int v){ a.store(v, memory_order_relaxed); }     // str
extern "C" void store_release(int v){ a.store(v, memory_order_release); }     // stlr
extern "C" void store_seq_cst(int v){ a.store(v, memory_order_seq_cst); }     // stlr

// ---------------- read-modify-write ----------------
extern "C" int fetch_add_relaxed()  { return a.fetch_add(1, memory_order_relaxed); }
extern "C" int fetch_add_acq_rel()  { return a.fetch_add(1, memory_order_acq_rel); }
extern "C" int fetch_add_seq_cst()  { return a.fetch_add(1, memory_order_seq_cst); }
extern "C" int exchange_seq_cst(int v) { return a.exchange(v, memory_order_seq_cst); }
extern "C" bool cas_seq_cst(int expected, int desired) {
    return a.compare_exchange_strong(expected, desired, memory_order_seq_cst);
}

// ---------------- standalone fences ----------------
extern "C" void fence_seq_cst() { std::atomic_thread_fence(memory_order_seq_cst); }
extern "C" void fence_acquire() { std::atomic_thread_fence(memory_order_acquire); }
extern "C" void fence_release() { std::atomic_thread_fence(memory_order_release); }
extern "C" void compiler_only_barrier() { asm volatile("" ::: "memory"); } // emits nothing!

// ---------------- hand-written barriers (AArch64 only) ----------------
#if defined(__aarch64__)
extern "C" void dmb_ish()   { asm volatile("dmb ish"   ::: "memory"); } // full, inner-shareable
extern "C" void dmb_ishld() { asm volatile("dmb ishld" ::: "memory"); } // orders loads before loads/stores
extern "C" void dmb_ishst() { asm volatile("dmb ishst" ::: "memory"); } // orders stores before stores
#endif

// ---------------- the two Preshing-test transactions on ARM ----------------
// Store-buffering (the thing your x86 program measures)
atomic<int> X, Y;
extern "C" int sb_thread_relaxed() {      // str, then ldr  -> can reorder on ARM (and x86)
    X.store(1, memory_order_relaxed);
    return Y.load(memory_order_relaxed);
}
extern "C" int sb_thread_seq_cst() {      // stlr, then ldar (ARMv8: still ordered w.r.t. each other)
    X.store(1, memory_order_seq_cst);
    return Y.load(memory_order_seq_cst);
}
extern "C" int sb_thread_fence() {        // str, dmb ish, ldr
    X.store(1, memory_order_relaxed);
    std::atomic_thread_fence(memory_order_seq_cst);
    return Y.load(memory_order_relaxed);
}

// Message passing
atomic<int> data, flag;
extern "C" void mp_writer_relaxed()  {
    data.store(1, memory_order_relaxed);
    flag.store(1, memory_order_relaxed);               // two plain str: can be reordered
}
extern "C" void mp_writer_release()  {
    data.store(1, memory_order_relaxed);
    flag.store(1, memory_order_release);               // str, then stlr
}
extern "C" int mp_reader_relaxed() {
    int f = flag.load(memory_order_relaxed);           // two plain ldr: can be reordered
    int d = data.load(memory_order_relaxed);
    return f * 10 + d;
}
extern "C" int mp_reader_acquire() {
    int f = flag.load(memory_order_acquire);           // ldar, then ldr
    int d = data.load(memory_order_relaxed);
    return f * 10 + d;
}
