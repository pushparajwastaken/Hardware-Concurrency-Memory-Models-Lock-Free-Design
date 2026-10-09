# Research Notes: CAS & Lock-Free Algorithms
 

## 📝 Introduction
This document serves as the complete research portfolio for the CAS / Lock-Free Algorithms role. It begins with the theoretical foundations of lock-free programming (CAS, ABA, Memory Reclamation) and transitions into empirical engineering (Phase 2 Mandate). No theory is accepted on faith; every claim is backed by working C++ code, benchmark results, and hardware metrics.

---

## Part 1: Theoretical Foundations

### 1. CAS (Compare-And-Swap)
CAS is a hardware-level atomic instruction, essential for lock-free programming. It takes three parameters:
- **Address:** The memory location to check.
- **Expected Value:** The value the address is expected to hold.
- **New Value:** The value to set if the expected value matches.

**Logic:** `if (*Address == Expected) { *Address = New; return true; } else return false;`

In C++, this is implemented via `std::atomic::compare_exchange_weak()` and `compare_exchange_strong()`. On x86, this maps to the `LOCK CMPXCHG` instruction.

### 2. Retry Loops
Because multiple threads compete for the same memory address, a CAS operation can fail if another thread modifies the value in between. Therefore, CAS must be wrapped in a loop:
```cpp
while (!counter.compare_exchange_weak(expected, desired)) {
    desired = expected + 1; // 'expected' is automatically updated on failure
}
```

## Part 2: Empirical Implementation & Findings (Phase 2 Mandate)

### 1. The MPMC Livelock Incident (A Real Engineering Struggle)

My initial implementation was a **Multi-Producer Multi-Consumer (MPMC)** lock-free ring buffer. The code compiled and ran fine with **2 threads**. However, the moment I scaled it to multi-core execution (**8 and 16 threads**), the benchmark **hung indefinitely**. The terminal froze mid-execution. I had to press `Ctrl + C` to kill it.

**Root Cause Analysis (Debugging Journey):**
After reviewing the code and consulting concurrency references, I identified the issue as a classic **livelock**:

- Producers would reserve the `tail_` slot via CAS, but get preempted by the OS before writing data into `buffer_[current_tail]`.
- Consumers would then spin indefinitely on stale `head_`/`tail_` values, waiting for data that was never published.
- Because every thread was stuck in a tight `while(!push())` / `while(!pop())` spin loop, the OS scheduler had no opportunity to preempt any thread. Every core was 100% busy doing nothing productive. **Classic starvation.**

**The Fix (Suggested and Implemented):**
I added `std::this_thread::yield()` to the failure paths in both `push()` and `pop()`. This was a critical fix.

```cpp
// In push() - when buffer is full
if (next_tail == head_.load(std::memory_order_acquire)) {
    std::this_thread::yield();  // <-- THE FIX
    return false;
}

// In pop() - when buffer is empty
if (current_head == tail_.load(std::memory_order_acquire)) {
    std::this_thread::yield();  // <-- THE FIX
    return false;
}
```
### Debugging Note: The Compiler Optimization Trap

My benchmark initially showed a **43.87x improvement** (250M ops/sec, 4 ns/op). These numbers were suspiciously high. I investigated and discovered that the compiler had **optimized away the entire consumer loop** because the popped value was never used. With `-O2`, the compiler deduced that the loop had no observable side effects and eliminated it entirely.

**The Fix:** I introduced an atomic `sink` variable and added `sink.fetch_add(val, std::memory_order_relaxed)` after every pop. This forced the compiler to keep the loop, since the value was now being consumed.

**After fix:** The realistic numbers are **11.02x improvement** (57.1M ops/sec, 17.5 ns/op). This is honest, reproducible, and reflects the true performance benefit of lock-free synchronization over mutex-based queues.

**Lesson Learned:** In benchmarking, always verify that the compiler is not optimizing away your test loop. This is a common pitfall in C++ benchmarking, and one of the reasons tools like Google Benchmark exist.

### ABA References

The ABA problem is not a theoretical curiosity. It is documented in:
- **Michael & Scott (PODC 1996)**: First formal treatment in lock-free queues.
- **Michael (IEEE TPDS 2004)**: Hazard pointers as a systematic solution.
- **SEI CERT CON09-C**: Industry coding standard with concrete examples.
- **Intel SDM Vol. 3A, §8.2**: Hardware-level memory ordering context.

My own ABA-safe implementation (`abaDemo.cpp`) uses tagged pointers and survives 4 threads x 100K operations without corruption. The naive version (without tags) crashes or livelocks under the same load.

### Key Takeaway:
 Lock-free programming trades kernel context switches for CPU cycle burning. It is a precision tool for low-latency systems, not a default replacement for mutexes. And always, always add yield() or backoff to your spin loops when threads outnumber cores.
