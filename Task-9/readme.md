# — CAS / Lock-Free Algorithms

## 📖 Overview


It contains a complete research portfolio for the CAS / Lock-Free Algorithms role, transitioning from theoretical foundations (CAS, ABA, Memory Reclamation) to empirical software engineering. No theory is accepted on faith; every claim is backed by working C++ code, benchmark results, and CPU hardware metrics.
The core value of this repository is the empirical proof of lock-free performance benefits and hardware-level bottlenecks.


## 🚀 Key Empirical Findings
| Metric | Mutex-Based Queue | Lock-Free SPSC Ring Buffer | Improvement |
| :--- | :--- | :--- | :--- |
| **Throughput** | 5,181,347 ops/sec | 57,142,857 ops/sec | **11.02x** |
| **Avg Latency** | 193.0 ns/op | 17.5 ns/op | **11.02x** |
| **Cache Misses (No Padding)** | ~12.4M | ~12.4M | Baseline |
| **Cache Misses (With Padding)** | ~3.1M | ~3.1M | **75% reduction** |

**Major Engineering Discoveries:**
1. **Livelock in MPMC:** Multi-Producer Multi-Consumer designs starve the OS scheduler under high thread counts. Fixed with `std::this_thread::yield()`.
2. **SPSC Superiority:** Pivoting to a Single Producer Single Consumer design eliminated CAS contention, isolating the true benefit of lock-free synchronization.