# Hardware Concurrency, Memory Models & Lock-Free Design

A 12-member student research and engineering project on how concurrent C++ code interacts with CPU execution, memory ordering, caches and synchronization primitives. Phase 1 produced research notes and small compiler/assembly experiments. Phase 2 ("From Theory to Silicon Proof") produced a store-buffering stress test, a measured false-sharing benchmark, a mutex/spinlock/atomic contention benchmark, and a ring-buffer report. This README states what the repository contains, what is backed by code and data, and what is not (see [§13](#13-limitations--known-gaps)).

> Scope note: this README was written from an audit of the repository contents. Anything not found in the repository is marked as such rather than assumed.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Why This Matters](#2-why-this-matters)
3. [Research Scope](#3-research-scope)
4. [Phase 1 — Research & Understanding](#4-phase-1--research--understanding)
5. [Phase 2 — From Theory to Silicon Proof](#5-phase-2--from-theory-to-silicon-proof)
6. [Experiments & Results](#6-experiments--results)
7. [Key Findings](#7-key-findings)
8. [Repository Structure](#8-repository-structure)
9. [Team Contributions](#9-team-contributions)
10. [How to Run](#10-how-to-run)
11. [Benchmark Methodology](#11-benchmark-methodology)
12. [Reproducibility](#12-reproducibility)
13. [Limitations & Known Gaps](#13-limitations--known-gaps)
14. [Engineering Lessons](#14-engineering-lessons)
15. [References](#15-references)
16. [Project Status](#16-project-status)
17. [Contribution / Team Note](#17-contribution--team-note)

---

## 1. Overview

**Objective.** Understand, and where possible measure, the path from application threads down to hardware:

```text
Application / Threads
        ↓
Atomic operations / Synchronization
        ↓
Memory ordering
        ↓
CPU execution
        ↓
Cache hierarchy → Cache coherence → Shared memory
        ↓
Performance
```

**Why it is hard.** Source-code order is not what the hardware executes: the compiler transforms code, the CPU executes out of order and buffers stores, and each core caches memory in lines that must be kept coherent. A program that "works on my machine" can still be wrong under the language memory model, and a correct program can still be slow because of where its variables sit in memory.

**Why high-level abstractions are not enough.** `std::atomic`, mutexes and queues hide these layers, but their correctness (memory orders) and cost (cache-line traffic, retries, blocking) are determined by them. The repository's research notes and experiments are organised around that chain.

---

## 2. Why This Matters

| Problem | Where it appears in this repository |
|---|---|
| Race conditions / lost updates | Non-atomic counter examples in [`Task-10` report](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §9.1 and [`task-8/research.md`](task-8/research.md) |
| Visibility / unexpected ordering | Store-buffering stress test, [`Task-4`](Task-4/store-buffering-stress-test.cpp); release/acquire vs relaxed notes in [`Task-2`](Task-2/cpu-reordering.pdf) |
| Cache contention, false sharing | Measured benchmark in [`Task-7`](Task-7/README.md) |
| Synchronization overhead | Mutex vs spinlock vs atomic counter in [`task-8`](task-8/analysis.md) |
| Lock-free design, latency/throughput | SPSC ring buffer and queue comparison reported in the [`Task-10` report](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Benchmark reliability | Methodology in [`Task-11`](Task-11/methodology/benchmark-methodology.md) |

---

## 3. Research Scope

Topics below are represented by files in the repository.

| Area | Topics | Location |
|---|---|---|
| CPU execution | Pipelining, hazards (RAW/WAR/WAW), register renaming, forwarding, superscalar, out-of-order execution, reorder buffer/retirement, speculation, branch prediction; compiler optimisation vs CPU scheduling | [`Task-2`](Task-2/) |
| Memory ordering | Relaxed / acquire / release / acq_rel / seq_cst, happens-before, fences, store buffering | [`Task-2`](Task-2/notes.md), [`Task-4`](Task-4/x86-memory-model.md) |
| Architecture | x86-64 TSO, `MFENCE`/`LFENCE`/`SFENCE`, locked operations, C++ order → x86 assembly mapping, MP and IRIW litmus tests | [`Task-4`](Task-4/) |
| Cache coherence | MESI, MOESI, MESIF/MOESIF/PMESI, snooping vs directory (literature review) | [`Task - 6.pdf`](Task%20-%206.pdf) |
| Synchronization | Mutex, spinlock, blocking, context switching, contention, CPU utilisation | [`task-8`](task-8/) |
| False sharing | Packed vs 64-byte padded vs truly shared counters | [`Task-7`](Task-7/) |
| Data structures | Ring buffer, SPSC/MPSC/MPMC, LMAX Disruptor concepts, waiting strategies | [`Task-10`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Performance | Benchmark methodology, warm-up, repetitions, percentiles, CPU frequency/thermal/affinity/NUMA (as a methodology document) | [`Task-11`](Task-11/) |

**Not found in the repository:** ARM/AArch64 memory-model material (only a one-sentence comparison in `Task-2/notes.md`), invalidate queues, load buffers, a CAS-loop implementation, ABA, wait-free algorithms, NUMA experiments, thermal or frequency measurements.

---

## 4. Phase 1 — Research & Understanding

### CPU execution and compiler vs CPU reordering — [`Task-2/`](Task-2/)
- [`notes.md`](Task-2/notes.md): ISA → microarchitecture, pipelining and stalls, data hazards, register renaming, forwarding, superscalar and out-of-order execution, reorder buffer and retirement, speculation and branch prediction, memory hierarchy, cache coherence vs memory ordering.
- [`cpu-reordering.pdf`](Task-2/cpu-reordering.pdf) (25 pages): write-up of the experiments below.
- Small experiments, each with source, `-O0`/`-O2` assembly and a short conclusion:
  - `experiment1` — `-O2` folds `foo(10,20)` to a constant at compile time. Compiler optimisation, not runtime out-of-order execution ([`experiment1.md`](Task-2/experiment1.md)).
  - `experiment2` — independent vs dependent arithmetic; out-of-order execution cannot remove true (RAW) dependencies ([`experiment2.md`](Task-2/experiment2.md)).
  - `experiment3` — `-O0` emits `jle`, `-O2` emits `cmovge`: the compiler can remove a branch before the CPU's predictor sees it ([`experiment3.md`](Task-2/experiment3.md)).
  - `experiment4` / `experiment4b` — release/acquire vs relaxed message passing (source only; see [§5.1](#51-race--memory-ordering-poc)).
  - Note: the PDF numbers these as Experiments 1–5; the file names differ (the branch experiment is `experiment3`, atomics are `experiment4*`).

### Memory ordering and x86-64 — [`Task-4/`](Task-4/)
- [`x86-memory-model.md`](Task-4/x86-memory-model.md): TSO ordering rules, fences (including a correction that `LFENCE` does not order memory visibility), locked/RMW operations, "a C++ memory order is a language contract, not an instruction mnemonic", MP and IRIW litmus tests, scope caveats.
- [`x86-atomics.cpp`](Task-4/x86-atomics.cpp) + [`x86-atomics-gcc14-x86_64.s`](Task-4/x86-atomics-gcc14-x86_64.s): recorded GCC 14.2 lowering — relaxed/acquire loads and relaxed/release stores are plain `MOV`; `seq_cst` store is `XCHG`; `fetch_add` is `LOCK XADD`; `seq_cst` fence is `lock or QWORD PTR [rsp], 0`.
- [`x86-memory-litmus-tests.cpp`](Task-4/x86-memory-litmus-tests.cpp): MP and IRIW function definitions (illustrations, not a harness).
- [`x86-64-memory-ordering-comparison-table.csv`](Task-4/x86-64-memory-ordering-comparison-table.csv), [`REVIEW-AND-CORRECTIONS.md`](Task-4/REVIEW-AND-CORRECTIONS.md), [`REAADME.md`](Task-4/REAADME.md) (file name as committed).

### Cache coherence — [`Task - 6.pdf`](Task%20-%206.pdf)
A 7-page literature review of MESI/MOESI and variants: state definitions, snooping vs directory, twelve true/false verified cases, read-miss walkthrough, reported results from cited studies. It contains no code or measurements of its own.

### Synchronization — [`task-8/research.md`](task-8/research.md)
Notes on race conditions, mutex, spinlock, blocking, context switching, contention and CPU utilisation, each with a cited paper, plus short example programs (listings inside the notes).

### Data structures — [`Task-10/`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf)
Ring buffer fundamentals, sequence numbers and safe reuse, SPSC/MPSC/MPMC, single-writer principle, LMAX Disruptor components (RingBuffer, Sequence, Sequencer, SequenceBarrier, WaitStrategy, gating sequences), waiting strategies, why a ring buffer needs release/acquire.

### Performance engineering — [`Task-11/`](Task-11/)
[`benchmark-methodology.md`](Task-11/methodology/benchmark-methodology.md) defines the standard (environment metadata, warm-up, repetitions, min/max/mean/P50/P90/P95/P99, CPU conditions, suspicious-result handling, definition of done). [`benchmark-template.csv`](Task-11/benchmark-template.csv) contains only a header row. [`cpu-and-hardware-concurrency.md`](Task-11/research/cpu-and-hardware-concurrency.md) is a short introduction that ends mid-section (unclosed code block after "Physical CPU Cores").

---

## 5. Phase 2 — From Theory to Silicon Proof

Summary against the four mandate items:

| # | Requirement | Repository evidence | Status |
|---|---|---|---|
| 1 | Prove the race in code | Runnable store-buffering test (memory ordering). Data-race lost-update programs exist only as listings in documents. | 🟡 Partial |
| 2 | Benchmark false sharing | Runnable benchmark with raw data, graph and environment record | ✅ Complete |
| 3 | Build a lock-free construct | SPSC ring buffer exists only as a listing in a PDF; no CAS loop; no executable source or tests | 🟡 Partial |
| 4 | 100% team knowledge transfer | Q&A checklist for one member; no evidence for the other eleven | 🟡 Partial |

### 5.1 Race / Memory Ordering PoC

**Store-buffering stress test (runnable)** — [`Task-4/store-buffering-stress-test.cpp`](Task-4/store-buffering-stress-test.cpp)
- C++20, two threads with relaxed atomics. Each thread stores to its own variable, then loads the other's; the main thread samples whether both loads saw `0` (`r0 == 0 && r1 == 0`), an outcome impossible under sequential consistency but allowed on x86-64 TSO.
- Documented result: **31,822 of 200,000 samples (~16%)** on an x86-64 VM with GCC 12.2.0 `-O2`. The raw output was not committed. The file states that the rate is machine-dependent and that zero observations would not disprove the model.
- Caveat: the worker threads run free while the main thread samples, so samples are not strictly paired litmus iterations. Treat the percentage as indicative.

**Release/acquire vs relaxed** — [`Task-2/experiment4.cpp`](Task-2/experiment4.cpp), [`experiment4b.cpp`](Task-2/experiment4b.cpp)
- Producer writes plain `data = 42` then sets an atomic flag with release (4) or relaxed (4b); consumer spins then prints `data`. Both print `42`. The PDF itself states this does not show the relaxed version is safe, and that it is not a clean demonstration because `data` is non-atomic. No failing outcome was produced.

**Data race (lost updates)** — listings only
- [`Task-10` report](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §9.1: `race_demo.cpp` (4 threads × 1,000,000 increments of a `volatile uint64_t`). Reported totals over five runs: 1,114,890; 1,362,566; 2,036,062; 1,570,796; 956,473 (expected 4,000,000).
- [`task-8/research.md`](task-8/research.md): 2 threads × 1,000,000 increments of a plain `int`; recorded outputs 1000000, 195582, 173522 (expected 2,000,000).
- No standalone `race_demo.cpp` or saved run output is in the repository.

### 5.2 False Sharing Benchmark

Source: [`Task-7/false-sharing.cpp`](Task-7/false-sharing.cpp) (Linux, C++20). Results: [`Task-7/measured-results/`](Task-7/measured-results/). Write-up: [`Task-7/README.md`](Task-7/README.md), [`false-sharing-results.md`](Task-7/false-sharing-results.md).

- **Packed:** 8-byte atomic counters back to back, eight per 64-byte line.
- **Padded:** one counter per 64-byte line (`alignas(64)` on each element).
- **True shared:** all threads increment one counter (control).
- All modes run `fetch_add(1, relaxed)`, 10,000,000 times per thread; threads pinned to CPUs 0..n-1 and released together from a start gate; the program verifies counter values and layout (addresses, stride, line count) at runtime.
- Thread counts: 1, 2, 4, 8, 9. Seven repetitions in shuffled order (fixed seed), 105 runs total.

Median throughput (million increments/s), 7 runs per cell:

| Threads | Packed | Padded | True shared | Padded / packed |
|---:|---:|---:|---:|---:|
| 1 | 136.2 | 131.4 | 132.7 | 0.96× |
| 2 | 123.2 | 110.3 | 123.5 | 0.89× |
| 4 | 64.8 | 227.0 | 65.6 | 3.50× |
| 8 | 43.4 | 417.1 | 43.8 | 9.62× |
| 9 | 49.6 | 439.4 | 42.2 | 8.86× |

Graph: [`false-sharing-graph.png`](Task-7/measured-results/false-sharing-graph.png). Disassembly confirms the hot loop in all modes is `lock addq $0x1,(%rdx)` ([`hot-loop-assembly.txt`](Task-7/measured-results/hot-loop-assembly.txt)).

**Limitations (from the authors' own notes and the environment record):** cloud KVM VM (Xeon Platinum 8573C, 9 vCPUs, cgroup quota of 8 CPUs); 38 of 105 runs saw cgroup throttling; the 9-thread row is over quota and spills packed counters onto a second line; `perf` was unavailable so cache-line transfers are inferred from the layout change, not counted; no bare-metal rerun yet.

A second, independent false-sharing figure appears in the Task-10 report (see [§6](#6-experiments--results)); its source and CSV are not in the repository.

### 5.3 Lock-Free Construct

- **Bounded SPSC ring buffer:** a C++ class template using `std::atomic` head/tail indices with release/acquire ordering and no mutex, 64-byte-aligned indices. It appears **only as a code listing** in [`Task-10/…Final_Report.pdf`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §9.4. The report names `spsc_ring_buffer.cpp`, `phase2_bench.cpp` and two result CSVs as "included", but **none of these files are in the repository**.
- **CAS loop:** Not found in the repository.
- **Correctness testing:** a checksum comparison (12,500,002,500,000 for 5,000,000 messages) is reported in the PDF. No test code is present.
- **Performance testing:** reported only (see [§6](#6-experiments--results)).
- Related but not a lock-free construct: [`task-8/locking-benchmark.cpp`](task-8/locking-benchmark.cpp) benchmarks a `std::atomic<int>` increment against a mutex and an `atomic_flag` spinlock. Task-7 uses `fetch_add`. These are hardware atomic read-modify-write operations, not CAS retry loops.

### 5.4 Team Knowledge Transfer

- [`Task-10` report](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §14 contains a 12-question walkthrough checklist and a 30-second explanation for Member 10.
- [`Task-4/REVIEW-AND-CORRECTIONS.md`](Task-4/REVIEW-AND-CORRECTIONS.md) records a review of Member 4's deliverable and leaves a project-specific sign-off explicitly open.
- [`Task-7/README.md`](Task-7/README.md) and [`false-sharing-results.md`](Task-7/false-sharing-results.md) explain method, results and limits in the author's words.
- No walkthrough records, Q&A logs or sign-offs showing completed KT for any member. Evidence of KT preparation exists for a minority of members only.

---

## 6. Experiments & Results

| Experiment | Hypothesis | Implementation | Measurement | Result |
|---|---|---|---|---|
| Store buffering | x86-64 allows a load to pass an older store to a different address | [`Task-4/store-buffering-stress-test.cpp`](Task-4/store-buffering-stress-test.cpp), runnable | `r0==0 && r1==0` count over 200,000 samples | Reported 31,822 (~16%); output not committed |
| Race PoC (lost updates) | Unsynchronised increments lose updates | Listings only: Task-10 PDF §9.1, `task-8/research.md` | 4 threads × 1M (PDF, 5 runs); 2 threads × 1M (notes, 3 runs) | All recorded totals below expected; no source file or raw output in repo |
| Release/acquire vs relaxed | Relaxed gives no synchronisation with `data` | [`Task-2/experiment4.cpp`](Task-2/experiment4.cpp), [`4b`](Task-2/experiment4b.cpp) | Printed value of `data` | Both print 42; no difference shown (the PDF says so) |
| False sharing (Task-7) | Padding to 64 B removes cache-line bouncing | [`Task-7/false-sharing.cpp`](Task-7/false-sharing.cpp) | Median Mops/s, 7 runs, 1–9 threads, pinned | Padded/packed 3.50× at 4 threads, 9.62× at 8; 0.96× and 0.89× at 1 and 2; packed ≈ true shared at 8 (43.4 vs 43.8) |
| False sharing (Task-10, reported) | Same | Not in repo (`phase2_bench.cpp`) | 2 threads, median of 3 runs of 1 s | Reported 241.296 vs 1,040.680 Mops/s (≈4.31×), AMD EPYC 9V74 VM, no pinning |
| Mutex / spinlock / atomic | Contention hurts spinlocks most | [`task-8/locking-benchmark.cpp`](task-8/locking-benchmark.cpp) | Mean time (ms) of 4 runs, 1M increments/thread, 1–8 threads | At 8 threads: mutex 558.75, spinlock 1572.00, atomic 119.50 ms (see data caveat below) |
| CAS | — | Not found in the repository | — | — |
| Ring buffer (reported) | SPSC ring has less overhead than a mutex queue | Listing only: Task-10 PDF §9.4 | 5M messages, 3 runs, checksum | Reported medians 12.055 vs 84.163 Mops/s (≈6.98×); checksums matched; no source or CSV in repo |

**Data caveat for `task-8`.** [`result.csv`](task-8/result.csv) has 42 of the 48 expected rows (run 3 at 1 thread and run 2 at 8 threads are absent). Averages in [`analysis.md`](task-8/analysis.md) for 2 and 4 threads match the CSV. The 1-thread and 8-thread averages do not: for example, at 8 threads the CSV gives mutex 567.33, spinlock 1666.33, atomic 122.33 ms (3 runs each) against 558.75, 1572.00, 119.50 in the analysis. The ordering (atomic < mutex < spinlock at 8 threads) holds either way.

---

## 7. Key Findings

### Experimentally demonstrated
Backed by code and data in this repository.

- With threads pinned and work fixed, per-thread counters on one 64-byte line scale badly while one-counter-per-line scales: **9.62× higher median throughput at 8 threads, 3.50× at 4** ([`Task-7`](Task-7/measured-results/summary.csv)). On this VM, packed counters performed about the same as a single truly shared counter at 8 threads.
- Padding gave no benefit at 1 thread and a median ~10% *loss* at 2 threads (wide, overlapping ranges; cause not established).
- Padded throughput at 8 threads was 3.2× its 1-thread value, not 8×.
- GCC (x86-64, `-O2`) emits plain `MOV` for relaxed/acquire loads and relaxed/release stores, `XCHG` for a `seq_cst` store and `LOCK XADD` for `fetch_add` ([`Task-4`](Task-4/x86-atomics-gcc14-x86_64.s)). This is a compiler observation, not an architectural guarantee.
- `-O2` can remove work or branches before the CPU sees them (constant folding, `cmov`) ([`Task-2`](Task-2/)).
- In the task-8 benchmark (Windows, i5-13500H), atomic increment was fastest and the spinlock slowest at 8 threads.
- The store-buffering outcome is observable on real hardware (reported in `Task-4`; reproducible from the committed source, though the rate varies by machine).

### Research-supported
Supported by cited sources, not experimentally demonstrated here.

- x86-64 is TSO: store→load reordering is allowed, other reorderings are not; `LFENCE` serialises instruction execution rather than ordering memory visibility ([`Task-4`](Task-4/x86-memory-model.md)).
- Release/acquire creates a happens-before relationship; relaxed does not ([`Task-2`](Task-2/notes.md), `Task-4`).
- MESI has four states; MOESI adds Owned; reported gains of MOESI and variants come from specific simulation studies and should not be generalised ([`Task - 6.pdf`](Task%20-%206.pdf)).
- Compiler optimisation, out-of-order execution and memory ordering are separate mechanisms ([`Task-2`](Task-2/cpu-reordering.pdf)).
- Spinlocks burn CPU while waiting; mutexes block and pay scheduling cost ([`task-8/research.md`](task-8/research.md)).
- The Disruptor combines a preallocated ring with sequences, barriers and consumer dependency graphs, and is not claimed to be always faster than queues ([`Task-10`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf)).

### Still unverified
Needs additional experiments or files.

- The SPSC ring buffer's correctness and its ≈6.98× advantage over a mutex queue (reported only).
- The data-race lost-update counts (no source file or saved output).
- The ≈4.31× false-sharing result on AMD EPYC and why it differs from Task-7's 0.89× at 2 threads (different hardware, thread pinning and run length; not investigated).
- That cache-line transfers cause the false-sharing slowdown (`perf c2c` not run; the mechanism is inferred from layout changes only).
- Any relaxed-ordering failure (no reordering outcome from `data`/`ready` was produced).
- Behaviour on ARM/AArch64, CAS contention, ABA, MPSC/MPMC.

---

## 8. Repository Structure

```text
Hardware-Concurrency-Memory-Models-Lock-Free-Design/
├── Task - 6.pdf                      # MESI/MOESI literature review (7 pages)
├── Task-2/                           # CPU reordering, ILP, compiler vs CPU, release/acquire
│   ├── notes.md, cpu-reordering.pdf  # notes and 25-page write-up
│   ├── add.cpp, add2.cpp, main.cpp   # early assembly-inspection programs
│   ├── experiment1–3.cpp / .md       # compiler optimisation, dependencies, branches
│   ├── experiment4.cpp, experiment4b.cpp, main2.cpp   # release/acquire vs relaxed
│   ├── *.s                           # generated -O0/-O2 assembly
│   └── *.exe                         # committed Windows binaries (add, add2, main2, experiment4, experiment4b)
├── Task-4/                           # x86-64 memory model
│   ├── x86-memory-model.md, REAADME.md, REVIEW-AND-CORRECTIONS.md
│   ├── x86-atomics.cpp, x86-atomics-gcc14-x86_64.s
│   ├── x86-memory-litmus-tests.cpp, store-buffering-stress-test.cpp
│   └── x86-64-memory-ordering-comparison-table.csv
├── Task-7/                           # False-sharing benchmark
│   ├── false-sharing.cpp, run-benchmark.sh, analyze.py, requirements.txt, .gitignore
│   ├── README.md, false-sharing-results.md, RESEARCH.md
│   └── measured-results/             # raw.csv, summary.csv, graph, environment.txt, benchmark.log, hot-loop-assembly.txt
├── task-8/                           # Mutex / spinlock / atomic contention (note lower-case folder name)
│   ├── locking-benchmark.cpp, locking-benchmark.exe
│   ├── result.csv, graph.png, raw_data.png
│   └── analysis.md, research.md
├── Task-10/
│   └── Ring_Buffer_LMAX_Disruptor_Final_Report.pdf
└── Task-11/                          # Performance-engineering methodology
    ├── README.md, benchmark-template.csv (header only)
    ├── methodology/benchmark-methodology.md
    └── research/cpu-and-hardware-concurrency.md
```

There are no folders for Tasks 1, 3, 5, 9 or 12, and no top-level build system, test directory or CI configuration.

---

## 9. Team Contributions

Folders and commit messages use member numbers (`member-2`, `member-4`, "Member 11", "Member 10 — Ananya Narula" in the PDF). Numbers 1–12 follow the roster order below; the member-to-folder link is explicit for Members 2, 4, 10 and 11 and inferred from roster order plus commit author for Members 6, 7 and 8. The commit author name or GitHub handle is given where it differs from the roster name.

| Member | Phase 1 Area | Phase 2 Work | Key Artifact |
|---|---|---|---|
| Khushi Kumari | Not verifiable from repository. | Not verifiable from repository. | — |
| Pushparaj Singh | CPU execution, compiler vs OoO, memory-ordering notes (`Task-2`). Also committed the Task-10 PDF and the folder renames. | Release/acquire vs relaxed example (source only). Repository owner. | [`Task-2/cpu-reordering.pdf`](Task-2/cpu-reordering.pdf), [`notes.md`](Task-2/notes.md) |
| Anand Kumar Sahni | Not verifiable from repository. | Not verifiable from repository. | — |
| Anshul Prajapati (`Anshul036`) | x86-64 memory model, fences, litmus tests, C++→assembly mapping | Store-buffering stress test | [`Task-4/x86-memory-model.md`](Task-4/x86-memory-model.md), [`store-buffering-stress-test.cpp`](Task-4/store-buffering-stress-test.cpp) |
| Devansh Vinayak | Not verifiable from repository. | Not verifiable from repository. | — |
| Gagan Chaurasia | MESI/MOESI cache coherence review (uploaded the PDF; folder number inferred) | Not verifiable from repository. | [`Task - 6.pdf`](Task%20-%206.pdf) |
| Harsh Gupta (`Harsh`) | False sharing and cache-line bouncing research | False-sharing benchmark, analysis script, raw data, graph | [`Task-7/`](Task-7/) |
| Paras Gupta (`paras gupta`, `Astro-peek`) | Mutex, spinlock, contention research | Mutex/spinlock/atomic contention benchmark (Windows) | [`task-8/analysis.md`](task-8/analysis.md), [`locking-benchmark.cpp`](task-8/locking-benchmark.cpp) |
| Preksha Wani | Not verifiable from repository. | Not verifiable from repository. | — |
| Ananya Narula | Ring buffer, SPSC/MPSC/MPMC, LMAX Disruptor (PDF names the author; committed by Pushparaj) | Race demo, false-sharing and SPSC ring-buffer listings with reported results; KT checklist | [`Task-10/…Final_Report.pdf`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Chhavi Sharma | Performance-engineering methodology, hardware-concurrency introduction | Benchmark template (header only); no benchmark data collected | [`Task-11/methodology/benchmark-methodology.md`](Task-11/methodology/benchmark-methodology.md) |
| Richa Bharti | Not verifiable from repository. | Not verifiable from repository. | — |

Five of twelve members have no identifiable contribution in the repository. This may reflect work stored elsewhere.

---

## 10. How to Run

All commands below were executed in a scratch copy of the repository on Ubuntu 24.04 with g++ 13.3.0 and Python 3. Everything compiled and ran as described. The one-off timing results below are from that check and are not committed evidence.

```bash
git clone https://github.com/pushparajwastaken/Hardware-Concurrency-Memory-Models-Lock-Free-Design.git
cd Hardware-Concurrency-Memory-Models-Lock-Free-Design
```

### Requirements
- Linux or WSL2 for `Task-7` (uses `sched_setaffinity`, `/sys`, `taskset`, `lscpu`, `ldd`); GCC 11+ with C++20.
- Any g++ with `-pthread` for the others. No JDK, no Java, no other dependencies. `matplotlib` only for `Task-7/analyze.py` (pinned to 3.10.8 in `requirements.txt`).

### Running the false-sharing benchmark (`Task-7`)

```bash
cd Task-7
bash run-benchmark.sh results-local 10000000 7 auto   # output dir, iterations/thread, repeats, thread counts (auto or e.g. 1,2,4,8)
pip install -r requirements.txt
python3 analyze.py results-local                      # writes summary.csv and a graph into results-local/
```

The script builds with `-O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic`, records the environment, and refuses to overwrite an existing `raw.csv`. If your cache line is not 64 bytes, run with `CACHE_LINE_BYTES=<size>`. Quick smoke test: `bash run-benchmark.sh results-test 200000 1 1,2`.

### Running the store-buffering stress test (`Task-4`)

```bash
cd Task-4
g++ -std=c++20 -O2 -pthread store-buffering-stress-test.cpp -o sb && ./sb
# prints: rounds sampled: 200000, store-buffering outcomes (r0==0 && r1==0): N
```

`N` varies by machine. The documented run reported 31,822; a spot-check on a different VM gave 8,550.

### Regenerating the x86 assembly (`Task-4`)

```bash
g++ -std=c++20 -O2 -S -masm=intel x86-atomics.cpp                 # compare with x86-atomics-gcc14-x86_64.s
g++ -std=c++20 -fsyntax-only x86-memory-litmus-tests.cpp
```

On GCC 13.3 the instruction selection matched the recorded output (`mov`, `xchg`, `lock xadd`, `lock or`).

### Running the compiler / ordering experiments (`Task-2`)

```bash
cd Task-2
g++ -O0 -S -masm=intel experiment1.cpp -o experiment1_O0.s
g++ -O2 -S -masm=intel experiment1.cpp -o experiment1_O2.s    # same pattern for experiment2 and experiment3
g++ -O2 -std=c++17 -pthread experiment4.cpp -o experiment4 && ./experiment4    # release/acquire; prints 42
g++ -O2 -std=c++17 -pthread experiment4b.cpp -o experiment4b && ./experiment4b # relaxed; prints 42
```

The assembly commands are the ones shown in `cpu-reordering.pdf`. The `-O2 -std=c++17 -pthread` build lines are not documented in the repository and are given here as working equivalents.

### Running the locking benchmark (`task-8`)

No build command is documented in the repository (the committed `.exe` is a Windows binary; the analysis states MinGW-w64 GCC). This compiled and ran with:

```bash
cd task-8
g++ -O2 -std=c++17 -pthread locking-benchmark.cpp -o locking-benchmark && ./locking-benchmark
```

It prints one time (ms) per method for 1, 2, 4 and 8 threads. The committed results were produced on Windows 11, not Linux.

### CAS experiment and ring buffer

Not found in the repository. The Task-10 PDF (Appendix B) lists build commands for `phase2_bench.cpp` and `race_demo.cpp`, but neither file is present.

---

## 11. Benchmark Methodology

What each dataset actually recorded:

| Item | Task-7 false sharing (measured) | task-8 locking (measured) | Task-10 report (reported only) | Task-4 stress test (reported only) |
|---|---|---|---|---|
| CPU | Xeon Platinum 8573C, KVM, 9 vCPUs (cgroup quota 8), 1 thread/core | i5-13500H, 2.60 GHz | AMD EPYC 9V74, KVM, 5 CPUs | Not recorded ("x86-64 VM") |
| Cache | 64 B line (sysfs); L1d 240 KiB (5 instances), L2 10 MiB (5), L3 260 MiB | Not recorded | 64 B line | Not recorded |
| OS | Ubuntu 24.04.3, kernel 6.18.44 | Windows 11 | Linux (x86_64) | Not recorded |
| Compiler / flags | g++ 13.3.0, `-O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic -DCACHE_LINE_BYTES=64` | MinGW-w64 GCC; version and flags not recorded | GCC 14.2, C++17, `-O2` | GCC 12.2.0, `-O2`, C++20 |
| Warm-up | 200,000 increments per thread per mode/count | None | One warm-up run | Not stated |
| Iterations | 10,000,000 per thread | 1,000,000 per thread | 5,000,000 messages (ring); 1 s per layout (false sharing) | 200,000 sampling rounds |
| Repetitions | 7, shuffled order, fixed seed | 4 | 3 | 1 |
| Threads | 1, 2, 4, 8, 9, pinned to CPU *i* | 1, 2, 4, 8, not pinned | 2 (false sharing); 1 producer + 1 consumer; not pinned | 2 |
| Unit / clock | Mops/s; `steady_clock` | Milliseconds (integer); `high_resolution_clock` | Mops/s | Count of outcomes |
| Statistics | Median, min, max, Q1, Q3 | Mean only | Median | Single count |
| Raw data in repo | Yes ([`raw.csv`](Task-7/measured-results/raw.csv)) | Partial (42/48 rows) | No | No |

**Missing or inconsistent across the project:**
- Task-11 sets a standard (P50/P90/P95/P99, CPU frequency, temperature, utilisation, affinity, NUMA) that no committed dataset follows. `benchmark-template.csv` has no data rows.
- CPU frequency, temperature and utilisation were not recorded for any run.
- `task-8` used no warm-up, no pinning and millisecond resolution (1-thread atomic runs are 5–7 ms).
- `task-8` compiler version, flags and thread-affinity are not recorded.
- Task-7 recorded cgroup throttling per run (38 of 105 runs affected) and discloses it.

---

## 12. Reproducibility

- **Best-documented path:** `Task-7`. Environment, build command, binary hash, raw CSV, log and disassembly are committed; `run-benchmark.sh` regenerates everything and `analyze.py` re-derives the summary. Running `analyze.py` on the committed `raw.csv` reproduces the medians in the table in [§5.2](#52-false-sharing-benchmark).
- **Expected output:** only `Task-7` has verified expected values, and they depend on the machine. Qualitatively, packed throughput should fall with thread count while padded rises; absolute numbers will differ, especially on a VM with CPU quotas.
- **Run on bare metal** (or at least without a CPU quota) before drawing hardware conclusions; the authors list `perf c2c record` as the next step.
- **Raw data locations:** [`Task-7/measured-results/`](Task-7/measured-results/), [`task-8/result.csv`](task-8/result.csv). None for Task-4 or Task-10 results.
- **Graph generation:** `python3 analyze.py <folder>` in `Task-7`. The `task-8` graphs are committed PNGs with no generating script; they plot the averages from `analysis.md`.
- **Known limits:** x86-64 only; committed `.exe` files are Windows binaries and are not needed to rebuild; Task-10 numbers cannot be reproduced because the source and CSVs are absent.

---

## 13. Limitations & Known Gaps

- **Phase 2 item 1:** the lost-update race is shown only as document listings with unsaved output; the one runnable ordering demo (store buffering) has no committed raw output and a free-running sampler.
- **Phase 2 item 3:** no CAS loop; the SPSC ring buffer has no executable source, no tests and no committed measurements. The Task-10 appendix lists files as "included" that are absent from the repository.
- **Phase 2 item 4:** KT evidence exists for Member 10 only (a question list); none for the others.
- **Benchmarks:** only Task-7 meets its own standard. `task-8` has an incomplete CSV and averages that disagree with it for 1 and 8 threads. No dataset records P90/P95/P99, frequency or temperature.
- **Hardware evidence:** Task-7 ran on a throttled VM with no hardware counters. Task-10 ran on a different VM with unpinned threads. No bare-metal data, no perf/HITM data, no ARM data.
- **Conflicting results:** padding helps 9.6× at 8 threads in Task-7 but did not help at 2 threads (0.89×); the Task-10 report claims 4.31× at 2 threads on other hardware. No experiment reconciles these.
- **Experiment depth:** `Task-2` atomics experiment shows no failure for relaxed ordering; `Task-4` litmus tests (MP, IRIW) are definitions, not run harnesses.
- **Coverage:** no work from five roster members is identifiable; no folders for Tasks 1, 3, 5, 9, 12; ARM/AArch64, ABA, wait-free, MPMC are not covered in code or notes.
- **Document quality:** `Task-11/research/cpu-and-hardware-concurrency.md` is truncated; `Task-4/REAADME.md` has a misspelt file name; `Task-2/notes.md` contains Obsidian image links (`![[Pasted image …]]`) whose images are not in the repository; the Task-6 references are aggregator links (Consensus), some with truncated titles.
- **Repository hygiene:** Windows `.exe` files are committed in `Task-2` and `task-8`.

---

## 14. Engineering Lessons

Each lesson is tied to the team's own work.

- **Source order is not the whole execution story.** `-O2` folded a call to a constant and replaced a branch with `cmov` ([`Task-2`](Task-2/)); on x86-64 a load can pass an earlier store, and the store-buffering test observed it ([`Task-4`](Task-4/)).
- **Memory ordering is not cache coherence.** Coherence keeps copies of one location consistent; ordering governs how operations on different locations become visible ([`Task-2/notes.md`](Task-2/notes.md), `Task - 6.pdf`).
- **A C++ memory order is not an instruction.** Several orders compile to the same x86 instruction, so "it works on x86" says little about portability ([`Task-4`](Task-4/x86-memory-model.md)).
- **Atomic does not mean fast.** Atomic increments on one cache line performed no better than a truly shared counter at 8 threads ([`Task-7`](Task-7/false-sharing-results.md)).
- **False sharing arises between different variables.** Each thread had its own counter, yet packed counters were up to ~9.6× slower than padded ones ([`Task-7`](Task-7/)).
- **Padding is not free and does not always help.** No gain at 1–2 threads, and 8× the memory for 8 counters.
- **Contention costs depend on the primitive.** At 8 threads the spinlock was slowest and the atomic fastest in the task-8 workload ([`task-8`](task-8/analysis.md)).
- **Benchmark method matters.** Pinning, shuffled run order, runtime layout checks, correctness checks and disclosure of CPU throttling shaped how far Task-7 could be trusted; the unrecorded conditions in other datasets limit what they show ([`Task-11`](Task-11/methodology/benchmark-methodology.md)).
- **Hardware observations need qualification.** VM, quota, compiler and CPU all appear in the authors' own caveats.
- **Not demonstrated by this repository:** "lock-free is faster" and CAS retry cost. The Task-10 report states it does not claim that a ring buffer or the Disruptor is always faster.

---

## 15. References

Taken from the repository's documents; see each file for full citations.

**x86 and CPU architecture** ([`Task-4/x86-memory-model.md`](Task-4/x86-memory-model.md))
- Intel® 64 Architecture Memory Ordering White Paper
- Owens et al., *A Better x86 Memory Model: x86-TSO* (2009)
- Intel SDM — `SFENCE`
- *The Semantics of x86 Multiprocessor Machine Code*

**C++ memory model and atomics**
- C++ standard `[atomics.order]`; GCC `__atomic` Builtins documentation; LLVM *Atomic Instructions and Concurrency Guide* (`Task-4`)
- cppreference: `std::memory_order`, multi-threaded executions and data races, `std::mutex`, `std::atomic`, `std::atomic<T>::is_lock_free` ([`Task-10`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf))
- Boost.Atomic, *Thread coordination using Boost.Atomic* (`Task-4`)

**Cache coherence** ([`Task - 6.pdf`](Task%20-%206.pdf))
- Alkhamisi 2022; Altwaijry & Alzahrani 2013; Amory & Ahmed 2021; Blanchet & Dupouy 2013; Dey & Nair 2014; Faeq & Omran 2021; Fotouhi et al. 2025; Ivanov & Nunna 2001; Kaur & Sulochana 2018; Kaushik et al. 2021; Kehagias & Raptis 2016; Komuravelli et al. 2014; Krishna & Rajeev 2025; Nair et al. 2021; Patil et al. 2019; Saraswat et al. 2021; Somarouthu 2025; Tiwari et al. 2014

**Locks, contention and blocking** ([`task-8/research.md`](task-8/research.md))
- Raynal & Taubenfeld (2022); Anderson (1990); Dinh et al. (2018); Li, Ding & Shen (2007); Wieder & Brandenburg (2013); Federico, Marotta & Quaglia (2025)

**Disruptor and ring buffers** ([`Task-10`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf))
- LMAX Exchange: Disruptor technical paper, User Guide, Disruptor Wizard, source repository
- Intel® 64 and IA-32 Architectures Optimization Reference Manual (false sharing)

**False sharing and benchmarking** ([`Task-7/RESEARCH.md`](Task-7/RESEARCH.md))
- Linux kernel documentation, *False Sharing*
- GCC documentation, *Built-in Functions for Memory Model Aware Atomic Operations*

---

## 16. Project Status

| Area | Status | Evidence |
|---|---|---|
| Phase 1 Research | 🟡 Partial | CPU execution, x86-64, MESI/MOESI, locks, ring buffer/Disruptor, methodology documented; no ARM/AArch64, CAS/ABA or wait-free material; five members' work not identifiable |
| Race PoC | 🟡 Partial | Runnable store-buffering test (`Task-4`); lost-update race only as listings; relaxed vs release/acquire shows no failure |
| False Sharing | ✅ Complete | `Task-7`: source, script, raw CSV (105 runs), summary, graph, environment, disassembly; VM caveats disclosed |
| Lock-Free Construct | 🟡 Partial | SPSC ring buffer listing and reported results in a PDF only; no CAS loop; no executable source or tests |
| Benchmarking | 🟡 Partial | `Task-7` complete; `task-8` data incomplete and inconsistent with its analysis; Task-10 numbers unverifiable; Task-11 standard not applied |
| KT | 🟡 Partial | Question checklist for Member 10; review notes for Member 4; no evidence for the rest |
| Final Integration | 🔴 Missing | No consolidated report, no cross-member results; `Task-11` template empty; no top-level documentation before this README |

---

## 17. Contribution / Team Note

This is a collaborative 12-member engineering and research project. Each member owned a numbered topic (`Task-N`), and the repository records their individual deliverables as submitted. Corrections to attribution or missing artifacts can be made by adding the files or contribution notes to the relevant `Task-N` folder.
