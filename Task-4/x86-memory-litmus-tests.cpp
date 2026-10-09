// Message-passing and IRIW examples for x86-64 / C++ atomics.
// Compile for inspection: g++ -std=c++20 -O2 -S -masm=intel x86-memory-litmus-tests.cpp
// These functions define operations; they are not a stress-test harness and do not
// promise that any particular outcome will occur in a finite run.
#include <atomic>

// Message passing: payload then publication flag.
std::atomic<int> payload{0};
std::atomic<int> ready{0};

void mp_writer_relaxed() {
    payload.store(1, std::memory_order_relaxed);
    ready.store(1, std::memory_order_relaxed);
}
int mp_reader_relaxed_payload() {
    if (ready.load(std::memory_order_relaxed) == 1)
        return payload.load(std::memory_order_relaxed); // 0 is permitted by C++ relaxed semantics
    return -1;
}
void mp_writer_release() {
    payload.store(1, std::memory_order_relaxed);
    ready.store(1, std::memory_order_release);
}
int mp_reader_acquire() {
    if (ready.load(std::memory_order_acquire) == 1)
        return payload.load(std::memory_order_relaxed); // must observe payload=1 after synchronizing acquire
    return -1;
}

// IRIW: independent writers; readers visit the two locations in opposite orders.
std::atomic<int> ix{0};
std::atomic<int> iy{0};

void iriw_writer_x_relaxed() { ix.store(1, std::memory_order_relaxed); }
void iriw_writer_y_relaxed() { iy.store(1, std::memory_order_relaxed); }
void iriw_writer_x_sc() { ix.store(1, std::memory_order_seq_cst); }
void iriw_writer_y_sc() { iy.store(1, std::memory_order_seq_cst); }

// With all four operations relaxed, the C++ abstract model permits the IRIW
// split-view outcome; x86 hardware ordering does not permit it for ordinary WB RAM.
void iriw_reader0_relaxed(int &rx0, int &ry0) {
    rx0 = ix.load(std::memory_order_relaxed);
    ry0 = iy.load(std::memory_order_relaxed);
}
void iriw_reader1_relaxed(int &ry1, int &rx1) {
    ry1 = iy.load(std::memory_order_relaxed);
    rx1 = ix.load(std::memory_order_relaxed);
}

// Use this set of functions together for the all-seq_cst IRIW variant.
void iriw_reader0_sc(int &rx0, int &ry0) {
    rx0 = ix.load(std::memory_order_seq_cst);
    ry0 = iy.load(std::memory_order_seq_cst);
}
void iriw_reader1_sc(int &ry1, int &rx1) {
    ry1 = iy.load(std::memory_order_seq_cst);
    rx1 = ix.load(std::memory_order_seq_cst);
}
