# Detailed Findings, Evidence Status and Reproduction Notes

> This is the former root `README.md`, moved here unchanged in substance so that the root README can be a short landing page. It has been corrected for the `Task1/` folder (previously reported as missing) and extended with a few re-check results, each marked "re-check". All relative links were rewritten for this location.

A 12-member student research and engineering project on how concurrent C++ code interacts with CPU execution, memory ordering, caches and synchronization primitives. Phase 1 produced research notes (CPU execution, x86-64 and ARM/AArch64 memory models, cache coherence, locks, CAS and ABA, ring buffers) and small compiler/assembly experiments. Phase 2 ("From Theory to Silicon Proof") produced a measured ARM/AArch64 litmus-test suite run on a real phone, store-buffering experiments, a measured false-sharing benchmark, a mutex/spinlock/atomic contention benchmark, CAS/ring-buffer/ABA programs, a ring-buffer report, and an evidence audit with a knowledge-transfer plan. This README states what the repository contains, what is backed by code and data, and what is not (see [§13](#13-limitations--known-gaps)).

> Scope note: this README was written from an audit of the repository contents at commit `86480a4` (2026-10-10). Anything not found in the repository is marked as such rather than assumed. The `Task-12` evidence audit was done earlier, at commit `415b759`, so some of its rows are out of date (see [§5.4](#54-team-knowledge-transfer)). The commit hashes could not be re-verified from the ZIP snapshot used for the second audit pass, which carries no `.git` history.

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

**Two architectures.** The repository covers x86-64 (strongly ordered, TSO) in `Task-4` and ARM/AArch64 (weakly ordered) in `Task5`. The x86 side is research notes, assembly and a store-buffering stress test. The ARM side is the one with measured hardware results: a litmus-test suite run on an Android phone with a MediaTek Dimensity 7400 Ultra (4× Cortex-A78 + 4× Cortex-A55), reported in [§5.1](#51-race--memory-ordering-poc).

---

## 2. Why This Matters

| Problem | Where it appears in this repository |
|---|---|
| Race conditions / lost updates | Runnable [`Task-9/level1_Basics/raceCondition.cpp`](../Task-9/level1_Basics/raceCondition.cpp); listings in the [`Task-10` report](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §9.1 and [`task-8/research.md`](../task-8/research.md) |
| Visibility / unexpected ordering | Measured ARM litmus tests (SB, MP, LB) with controls, [`Task5`](../Task5-ARM_AArch-Memory-Model/README.md); store-buffering sources in [`task_3`](../task_3/practical/store_buffering.cpp) and [`Task-4`](../Task-4/store-buffering-stress-test.cpp); release/acquire vs relaxed notes in [`Task-2`](../Task-2/cpu-reordering.pdf) |
| Cache contention, false sharing | Measured benchmark in [`Task-7`](../Task-7/README.md) |
| Synchronization overhead | Mutex vs spinlock vs atomic counter in [`task-8`](../task-8/analysis.md) |
| CAS retries, ABA, lock-free design | CAS counter, tagged-pointer stack and SPSC ring buffer in [`Task-9`](../Task-9/readme.md); ring-buffer and queue comparison reported in the [`Task-10` report](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Benchmark reliability | Methodology in [`Task-11`](../Task-11/methodology/benchmark-methodology.md); claim-by-claim audit in [`Task-12`](../Task-12/evidence-matrix.md) |

---

## 3. Research Scope

### Task map

| Task | Folder | Roster member | Topic | Notes |
|---|---|---|---|---|
| 1 | [`Task1`](../Task1/) | Khushi Kumari (inferred from roster order and folder number; no commit data in the snapshot) | Research foundations: memory hierarchy, concurrency vs parallelism, false sharing, memory ordering, lock-free design | Two Word documents, a concept map and four demo programs; no folder README; no measured output committed |
| 2 | [`Task-2`](../Task-2/) | Pushparaj Singh | CPU execution, compiler vs CPU reordering | Notes, PDF, experiments 1–4b |
| 3 | [`task_3`](../task_3/) | Anand Kumar Sahni (commit author; folder number inferred) | Store buffers and memory ordering | Theory, store-buffering source, case study; results file is an empty template |
| 4 | [`Task-4`](../Task-4/) | Anshul Prajapati | x86-64 memory model | Notes, assembly, litmus definitions, stress test |
| 5 | [`Task5-ARM_AArch-Memory-Model`](../Task5-ARM_AArch-Memory-Model/) | Devansh Vinayak (inferred; committed as `Glazybyte`) | ARM/AArch64 memory model | Measured litmus tests on a phone, assembly, plots |
| 6 | [`Task - 6.pdf`](../Task%20-%206.pdf) | Gagan Chaurasia (inferred) | MESI/MOESI cache coherence | Literature review |
| 7 | [`Task-7`](../Task-7/) | Harsh Gupta | False sharing | Measured benchmark |
| 8 | [`task-8`](../task-8/) | Paras Gupta | Mutex / spinlock / atomic contention | Benchmark and analysis |
| 9 | [`Task-9`](../Task-9/) | Preksha Wani | CAS and lock-free algorithms | CAS counter, SPSC ring buffer, ABA demo, race demo |
| 10 | [`Task-10`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) | Ananya Narula (author named in the PDF) | Ring buffer, Disruptor | PDF report only |
| 11 | [`Task-11`](../Task-11/) | Chhavi Sharma | Performance-engineering methodology | Methodology and an empty template |
| 12 | [`Task-12`](../Task-12/) | Richa Bharti (file headers say "Member 12") | Evidence, documentation, knowledge transfer | Evidence matrix, bibliography, contribution ledger, KT plan |

### Topics

Topics below are represented by files in the repository.

| Area | Topics | Location |
|---|---|---|
| CPU execution | Pipelining, hazards (RAW/WAR/WAW), register renaming, forwarding, superscalar, out-of-order execution, reorder buffer/retirement, speculation, branch prediction; compiler optimisation vs CPU scheduling | [`Task-2`](../Task-2/) |
| Memory ordering | Memory-model concepts, relaxed / acquire / release / acq_rel / seq_cst, happens-before, fences, store buffering | [`Task-2`](../Task-2/notes.md), [`task_3`](../task_3/), [`Task-4`](../Task-4/x86-memory-model.md), [`Task5`](../Task5-ARM_AArch-Memory-Model/README.md) |
| x86-64 | TSO, `MFENCE`/`LFENCE`/`SFENCE`, locked operations, C++ order → x86 assembly mapping, MP and IRIW litmus tests | [`Task-4`](../Task-4/) |
| ARM/AArch64 | Weak ordering, `DMB`/`DSB`/`ISB`, `LDAR`/`STLR`, `LDXR`/`STXR`, LSE atomics (`LDADDAL`, `SWPAL`, `CASAL`), C++ order → AArch64 assembly mapping, SB/MP/LB litmus tests measured on hardware | [`Task5`](../Task5-ARM_AArch-Memory-Model/) |
| Cache coherence | MESI, MOESI, MESIF/MOESIF/PMESI, snooping vs directory (literature review) | [`Task - 6.pdf`](../Task%20-%206.pdf) |
| Synchronization | Mutex, spinlock, blocking, context switching, contention, CPU utilisation | [`task-8`](../task-8/) |
| False sharing | Packed vs 64-byte padded vs truly shared counters | [`Task-7`](../Task-7/) |
| CAS and lock-free | CAS retry loop, ABA and tagged pointers, SPSC ring buffer, MPMC livelock (narrative), memory-reclamation references | [`Task-9`](../Task-9/) |
| Data structures | Ring buffer, SPSC/MPSC/MPMC, LMAX Disruptor concepts, waiting strategies | [`Task-10`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Performance | Benchmark methodology, warm-up, repetitions, percentiles, CPU frequency/thermal/affinity/NUMA (as a methodology document) | [`Task-11`](../Task-11/) |
| Evidence and KT | 30-claim evidence matrix, 24-entry source log, contribution ledger, proposed KT sessions | [`Task-12`](../Task-12/) |

**Not found in the repository:** invalidation queues, load buffers, hazard-pointer / epoch / RCU implementations, a deterministic ABA reproducer, the MPMC ring-buffer code, wait-free algorithms, NUMA experiments, formal-model runs (rmem, herd7), thermal or frequency measurements, and the x86 paired store-buffering litmus test that used to be in `Task5` (moved out of this repository).

---

## 4. Phase 1 — Research & Understanding

### Research foundations — [`Task1/`](../Task1/)
- [`Hardware_Concurrency_Research1.docx`](../Task1/Hardware_Concurrency_Research1.docx): research note (about 9,000 characters of text) on the CPU→cache→RAM hierarchy, concurrency vs parallelism, a one-paragraph MESI introduction, false sharing and how to test it, coherence vs memory model, data races, acquire/release, CAS, what "lock-free" means, and ring buffers / the LMAX Disruptor. It states that it is a research foundation and not a claim that benchmarks were run. It contains a small release/acquire publication pattern (a code listing, not a runnable program).
- [`Hardware_Concurrency_References.docx`](../Task1/Hardware_Concurrency_References.docx): eight sources with URLs (cppreference memory order and atomics, the Linux kernel false-sharing page, Intel SDM, Arm Architecture Reference Manuals, the LMAX Disruptor repository, Herb Sutter's lock-free article, the ISO C++ draft on data races). Link validity was not checked.
- [`Concurrency_Concept_Map.png`](../Task1/Concurrency_Concept_Map.png): a six-box concept map separating correctness (memory model, safe communication, atomic / lock-free design) from performance (cache behaviour, false sharing, optimisation and measurement). No source or generator is recorded.
- Four demo programs, each with a header comment naming the concept: [`01_memory_hierarchy.cpp`](../Task1/01_memory_hierarchy.cpp) (latency vs working-set size, row-major vs column-major traversal, cache-line effect), [`02_false_sharing.cpp`](../Task1/02_false_sharing.cpp) (one thread vs many; adjacent vs 64-byte padded atomic counters, 100,000,000 iterations), [`03_memory_ordering.cpp`](../Task1/03_memory_ordering.cpp) (plain-counter data race, store-buffer litmus test with relaxed vs `seq_cst`, release/acquire publication), [`04_lock_free.cpp`](../Task1/04_lock_free.cpp) (CAS counter vs mutex counter, a Treiber stack that never frees nodes, an SPSC ring buffer vs a mutex queue).
- Status (re-check): all four programs compile with g++ 13.3.0 (`-O2 -std=c++17 -pthread`). They were **not run** in the re-check, and **no output, results file or README is committed in the folder**, so none of their behaviour is evidenced by committed data. `04_lock_free.cpp` states that it never frees popped nodes to avoid ABA; it therefore does not demonstrate ABA or reclamation.
- Not linked from the previous root README, and not covered by the `Task-12` audit.

### CPU execution and compiler vs CPU reordering — [`Task-2/`](../Task-2/)
- [`notes.md`](../Task-2/notes.md): ISA → microarchitecture, pipelining and stalls, data hazards, register renaming, forwarding, superscalar and out-of-order execution, reorder buffer and retirement, speculation and branch prediction, memory hierarchy, cache coherence vs memory ordering.
- [`cpu-reordering.pdf`](../Task-2/cpu-reordering.pdf) (25 pages): write-up of the experiments below.
- Small experiments, each with source, `-O0`/`-O2` assembly and a short conclusion:
  - `experiment1` — `-O2` folds `foo(10,20)` to a constant at compile time. Compiler optimisation, not runtime out-of-order execution ([`experiment1.md`](../Task-2/experiment1.md)).
  - `experiment2` — independent vs dependent arithmetic; out-of-order execution cannot remove true (RAW) dependencies ([`experiment2.md`](../Task-2/experiment2.md)).
  - `experiment3` — `-O0` emits `jle`, `-O2` emits `cmovge`: the compiler can remove a branch before the CPU's predictor sees it ([`experiment3.md`](../Task-2/experiment3.md)).
  - `experiment4` / `experiment4b` — release/acquire vs relaxed message passing (source only; see [§5.1](#51-race--memory-ordering-poc)).
  - Note: the PDF numbers these as Experiments 1–5; the file names differ (the branch experiment is `experiment3`, atomics are `experiment4*`).

### Store buffers and memory ordering — [`task_3/`](../task_3/)
- [`theory/theoretical-background.md`](../task_3/theory/theoretical-background.md): store buffers, instruction reordering, visibility, the five memory orders, fences, the store-buffering test, and why coherence does not replace ordering. About one page, no citations inside the file.
- [`practical/store_buffering.cpp`](../task_3/practical/store_buffering.cpp): C++20, two worker threads plus the main thread on a 3-party `std::barrier`. Each round the main thread resets `X` and `Y`, both workers run `store; load` on the other's variable (relaxed or `seq_cst`, chosen on the command line), and main counts `r1 == 0 && r2 == 0`. Rounds are paired, which the `Task-4` stress test is not. No pinning, no warm-up, no timing.
- [`experiment/results.md`](../task_3/experiment/results.md): compile and run commands and the expected interpretation. The "Actual Results" section is a placeholder that tells the reader to record their own output. **No measured result is committed.**
- [`case-study/case-study.md`](../task_3/case-study/case-study.md): a producer/consumer market-data scenario that motivates release/acquire. [`references/references.md`](../task_3/references/references.md): seven links (cppreference, Intel SDM, Linux kernel memory-barriers and false-sharing docs, the C++ standard page, Preshing).
- The folder README links a `diagrams/` folder that does not exist.

### Memory ordering and x86-64 — [`Task-4/`](../Task-4/)
- [`x86-memory-model.md`](../Task-4/x86-memory-model.md): TSO ordering rules, fences (including a correction that `LFENCE` does not order memory visibility), locked/RMW operations, "a C++ memory order is a language contract, not an instruction mnemonic", MP and IRIW litmus tests, scope caveats.
- [`x86-atomics.cpp`](../Task-4/x86-atomics.cpp) + [`x86-atomics-gcc14-x86_64.s`](../Task-4/x86-atomics-gcc14-x86_64.s): recorded GCC 14.2 lowering — relaxed/acquire loads and relaxed/release stores are plain `MOV`; `seq_cst` store is `XCHG`; `fetch_add` is `LOCK XADD`; `seq_cst` fence is `lock or QWORD PTR [rsp], 0`.
- [`x86-memory-litmus-tests.cpp`](../Task-4/x86-memory-litmus-tests.cpp): MP and IRIW function definitions (illustrations, not a harness).
- [`x86-64-memory-ordering-comparison-table.csv`](../Task-4/x86-64-memory-ordering-comparison-table.csv), [`REVIEW-AND-CORRECTIONS.md`](../Task-4/REVIEW-AND-CORRECTIONS.md), [`REAADME.md`](../Task-4/REAADME.md) (file name as committed).

### ARM/AArch64 memory model — [`Task5-ARM_AArch-Memory-Model/`](../Task5-ARM_AArch-Memory-Model/)
- [`README.md`](../Task5-ARM_AArch-Memory-Model/README.md): the full write-up. A background section (x86 vs ARM reordering table, which C++ constructs become which AArch64 instructions), the three litmus tests and their variants, build and run steps for Termux, the test environment, an unpinned terminal transcript, the pinned per-CPU-pair results, an analysis, caveats and references.
- [`arm_memory_model.md`](../Task5-ARM_AArch-Memory-Model/arm_memory_model.md): short concept notes (what a memory model is, why ordering matters, ARM as a weak model, `DMB`/`DSB`/`ISB`, `LDAR`/`STLR`, acquire/release, a caution against labelling ARM as PSO/RCsc/RCpc).
- [`arm-litmus.cpp`](../Task5-ARM_AArch-Memory-Model/arm-litmus.cpp) (417 lines): the SB, MP and LB harness. [`plot_litmus.py`](../Task5-ARM_AArch-Memory-Model/plot_litmus.py): plots and the weak-outcome table.
- [`arm-atomics.cpp`](../Task5-ARM_AArch-Memory-Model/arm-atomics.cpp) with [`arm-atomics.s`](../Task5-ARM_AArch-Memory-Model/arm-atomics.s) (ARMv8.0, GCC 13) and [`arm-atomics-lse.s`](../Task5-ARM_AArch-Memory-Model/arm-atomics-lse.s) (`-march=armv8.1-a`, LSE atomics): one tiny function per atomic operation, for reading the assembly.
- [`litmus_results/`](../Task5-ARM_AArch-Memory-Model/litmus_results/) and [`plots/`](../Task5-ARM_AArch-Memory-Model/plots/): raw data and eight figures. Results are in [§5.1](#51-race--memory-ordering-poc).

### Cache coherence — [`Task - 6.pdf`](../Task%20-%206.pdf)
A 7-page literature review of MESI/MOESI and variants: state definitions, snooping vs directory, twelve true/false verified cases, read-miss walkthrough, reported results from cited studies. It contains no code or measurements of its own.

### Synchronization — [`task-8/research.md`](../task-8/research.md)
Notes on race conditions, mutex, spinlock, blocking, context switching, contention and CPU utilisation, each with a cited paper, plus short example programs (listings inside the notes).

### CAS and lock-free algorithms — [`Task-9/`](../Task-9/)
- [`lockFree.md`](../Task-9/lockFree.md): CAS definition and retry loop (Part 1); then a first-person account of an MPMC ring-buffer livelock, a benchmark that was too good (43.87×) because of a compiler effect, and the final 11.02× result (Part 2). [`readme.md`](../Task-9/readme.md) is a one-page summary of the same results.
- [`casCounter.cpp`](../Task-9/casCounter.cpp): `compare_exchange_weak` loop on a shared counter, counts retries, appends a row to [`cas-results.csv`](../Task-9/cas-results.csv).
- [`lockFreeRingBuffer.hpp`](../Task-9/lockFreeRingBuffer.hpp): single-producer / single-consumer ring buffer, release/acquire on `head_`/`tail_`, 64-byte-aligned indices. [`ringBufferBench.cpp`](../Task-9/ringBufferBench.cpp) compares it with a mutex + condition-variable queue and writes [`ringBufferBench.csv`](../Task-9/ringBufferBench.csv).
- [`abaDemo.cpp`](../Task-9/abaDemo.cpp): defines a naive Treiber stack and a 16-bit-tagged-pointer stack, but runs **only the tagged one**. [`abaBenchMarkAnalysis.md`](../Task-9/abaBenchMarkAnalysis.md), [`abaReferences.md`](../Task-9/abaReferences.md), [`abaTextImage.txt`](../Task-9/abaTextImage.txt) (ASCII diagram of the ABA sequence).
- [`level1_Basics/`](../Task-9/level1_Basics/): [`raceCondition.cpp`](../Task-9/level1_Basics/raceCondition.cpp) (4 threads × 1,000,000 plain increments) and [`sharedMemory.cpp`](../Task-9/level1_Basics/sharedMemory.cpp) (global vs per-thread-stack addresses), each with a screenshot. Three more screenshots are in [`assets/`](../Task-9/assets/).

### Data structures — [`Task-10/`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf)
Ring buffer fundamentals, sequence numbers and safe reuse, SPSC/MPSC/MPMC, single-writer principle, LMAX Disruptor components (RingBuffer, Sequence, Sequencer, SequenceBarrier, WaitStrategy, gating sequences), waiting strategies, why a ring buffer needs release/acquire.

### Performance engineering — [`Task-11/`](../Task-11/)
[`benchmark-methodology.md`](../Task-11/methodology/benchmark-methodology.md) defines the standard (environment metadata, warm-up, repetitions, min/max/mean/P50/P90/P95/P99, CPU conditions, suspicious-result handling, definition of done). [`benchmark-template.csv`](../Task-11/benchmark-template.csv) contains only a header row. [`cpu-and-hardware-concurrency.md`](../Task-11/research/cpu-and-hardware-concurrency.md) is a short introduction that ends mid-section (unclosed code block after "Physical CPU Cores").

### Evidence audit and knowledge transfer — [`Task-12/`](../Task-12/)
- [`evidence-matrix.md`](../Task-12/evidence-matrix.md): 30 technical claims traced to source, repository location, PoC, test, observed result and status. Tally: 1 verified from source, 2 verified from code, 3 executed, 0 independently reproduced, 12 partial, 12 unverified. Also a benchmark-record table and a prioritised action list.
- [`sources.md`](../Task-12/sources.md): 24 bibliography entries with a verification status, plus a list of missing authoritative references and of citation problems inside the repository.
- [`team-contributions.md`](../Task-12/team-contributions.md): a per-member ledger of assigned work, artifacts, evidence, outstanding work and confidence.
- [`kt-plan.md`](../Task-12/kt-plan.md): a proposed ten-session knowledge-transfer plan (about ten hours). It states that no session has been held.
- The auditor's own runs were on a 1-vCPU VM, so no cross-core behaviour could appear and no claim was marked reproduced. The files refer to themselves as `docs/member-12/`; they are committed as `Task-12/`.

---

## 5. Phase 2 — From Theory to Silicon Proof

Summary against the four mandate items:

| # | Requirement | Repository evidence | Status |
|---|---|---|---|
| 1 | Prove the race in code | **ARM litmus suite run on a phone** (`Task5`): relaxed SB and MP produce the weak outcome, ordered variants never do, with raw data and figures. Runnable lost-update program in `Task-9` (one recorded run). Store-buffering sources in `task_3` (no results) and `Task-4` (harness questioned by the audit). | ✅ Complete (memory-ordering variant, on ARM hardware) |
| 2 | Benchmark false sharing | Runnable benchmark with raw data, graph and environment record | ✅ Complete |
| 3 | Build a lock-free construct | Runnable SPSC ring buffer, CAS retry-loop counter and tagged-pointer stack in `Task-9`, with CSVs. No correctness test, no multi-run data, no ABA reproducer, no MPMC code | 🟡 Partial |
| 4 | 100% team knowledge transfer | A 10-session KT plan (proposed, none held), Q&A checklist for one member, review note for another | 🟡 Partial |

### 5.1 Race / Memory Ordering PoC

**ARM/AArch64 litmus tests on hardware (runnable, raw data committed)** — [`Task5-ARM_AArch-Memory-Model/arm-litmus.cpp`](../Task5-ARM_AArch-Memory-Model/arm-litmus.cpp)
- Three classic tests, each with two threads, a spin barrier to start them together, a random start offset (0 to 127 `nop` steps) to scan alignments, and one `(r0, r1)` outcome recorded per run. The **weak outcome** is the one sequential consistency can never produce.
  - **SB** (store buffering): `x = 1; r0 = y` ∥ `y = 1; r1 = x`. Weak: `r0 == 0 && r1 == 0`.
  - **MP** (message passing): `data = 1; flag = 1` ∥ `r0 = flag; r1 = data`. Weak: `r0 == 1 && r1 == 0`.
  - **LB** (load buffering): `r0 = x; y = 1` ∥ `r1 = y; x = 1`. Weak: `r0 == 1 && r1 == 1`.
- Ten variants: SB relaxed, release/acquire, `seq_cst`, relaxed + `dmb ish`; MP relaxed, release/acquire, relaxed + fences (`dmb`, `dmb ishld`), writer-release-only (plain reader); LB relaxed, store-release.
- **Setup:** 2,000,000 iterations per test, one run per variant, per CPU pair. Pairs: CPUs 0 & 1, 6 & 7 and 0 & 7, pinned with `sched_setaffinity`. One earlier unpinned run is transcribed in the README.
- **Hardware:** MediaTek Dimensity 7400 Ultra 5G, 4× Cortex-A78 at 2.6 GHz + 4× Cortex-A55 at 2.0 GHz (ARMv8.2-A), Android with Termux, `clang++ -O2 -std=c++17 -pthread`.
- **Weak-outcome rate** (count of 2,000,000 in brackets; 🟩 = never seen):

| Test | Variant | CPUs 0 & 1 | CPUs 6 & 7 | CPUs 0 & 7 |
|---|---|---:|---:|---:|
| SB | relaxed (`str; ldr`) | 11.9253% (238,506) | 21.3815% (427,629) | 17.8945% (357,890) |
| SB | release/acquire, `seq_cst`, relaxed + `dmb ish` | 🟩 0 | 🟩 0 | 🟩 0 |
| MP | relaxed (`str; str` / `ldr; ldr`) | 0.5653% (11,306) | 2.5596% (51,192) | 0.6633% (13,266) |
| MP | release/acquire; relaxed + fences | 🟩 0 | 🟩 0 | 🟩 0 |
| MP | writer release only (plain reader) | 0.0040% (79) | 2.1524% (43,049) | 0.0006% (11) |
| LB | relaxed; store-release | 🟩 0 | 🟩 0 | 🟩 0 |

- Unpinned run (README transcript only, no raw files): SB relaxed 414,980 (20.7490%), MP relaxed 69,897 (3.4949%), MP writer-release-only 35,202 (1.7601%); all other variants 0.
- What the authors conclude: only the unordered variants break; anything with `stlr`/`ldar` on both sides or a `dmb` stayed at zero on all three pairs; writer-release-only is the trap, because the plain reader can still reorder its two loads, and it leaks heavily on one pair and barely on the others, so testing a single pair could make that code look safe. LB never appears even when relaxed. The architecture allows it, so zero means "not seen on this chip", not "forbidden".
- The authors also note that ARMv8 hardware orders a release store before a later acquire load, which is stricter than C++ requires, so the zero for SB release/acquire says nothing about the C++ guarantee.
- Raw data: [`litmus_results/`](../Task5-ARM_AArch-Memory-Model/litmus_results/) (`summary.txt` plus one folder per CPU pair and one sub-folder per variant, each with `reorders_vs_runs.txt` and `reorders_per_second.txt`). Figures in [`plots/`](../Task5-ARM_AArch-Memory-Model/plots/): cumulative and per-second weak outcomes for SB, MP and LB, a rate-by-instruction chart and an outcome-mix chart.
- Limits that the data cannot remove: one run per variant and pair; the outcome mix is skewed by which thread starts first, so compare rates, not mixes; no warm-up phase; CPU frequency, temperature and background load not recorded; no console log of the pinned runs, so a pinning failure (Android can refuse affinity changes and the program only prints a warning) would not be visible. The very different runtimes per pair (about 0.55–0.65 s on 6 & 7, 0.79–0.87 s on 0 & 7, 1.03–1.28 s on 0 & 1) suggest pinning took effect, but that is an inference. Which CPU numbers belong to the A55 and which to the A78 cluster is not recorded either; on most phones CPUs 0–3 are the little cores and 4–7 the big ones, which would make 0 & 1 a little pair, 6 & 7 a big pair and 0 & 7 a cross-cluster pair. The README gives a `sysfs` command to check.
- Not done: no run under the formal model (rmem), no repeat runs, no second device.

**Store-buffering experiment, paired (runnable, no results committed)** — [`task_3/practical/store_buffering.cpp`](../task_3/practical/store_buffering.cpp)
- C++20; counts rounds where both loads see `0`, in relaxed or `seq_cst` mode. Each round is reset and released through a barrier, so rounds are paired. Threads are not pinned and no CPU is recorded. [`results.md`](../task_3/experiment/results.md) has no measured output. The file states that an observation of zero does not disprove the model.

**Store-buffering stress test, x86-64 (runnable, questioned)** — [`Task-4/store-buffering-stress-test.cpp`](../Task-4/store-buffering-stress-test.cpp)
- C++20, two threads with relaxed atomics. Each thread stores to its own variable, then loads the other's; the main thread samples whether both loads saw `0`, an outcome impossible under sequential consistency but allowed on x86-64 TSO.
- Documented result: **31,822 of 200,000 samples (~16%)** on an x86-64 VM with GCC 12.2.0 `-O2`. The raw output was not committed.
- The `Task-12` audit ([E06](../Task-12/evidence-matrix.md)) found that the harness cannot tell store buffering from its own bookkeeping: the main thread resets the shared variables while the workers run, so `(0,0)` is reachable under sequential consistency too. An all-`seq_cst` control on its 1-vCPU VM gave counts of the same order as the original code. The audit recommends retiring the ~16% figure. This README does not treat it as evidence of x86 store buffering.

**Release/acquire vs relaxed** — [`Task-2/experiment4.cpp`](../Task-2/experiment4.cpp), [`experiment4b.cpp`](../Task-2/experiment4b.cpp)
- Producer writes plain `data = 42` then sets an atomic flag with release (4) or relaxed (4b); consumer spins then prints `data`. Both print `42`. The PDF itself states this does not show the relaxed version is safe, and that it is not a clean demonstration because `data` is non-atomic. No failing outcome was produced. On x86-64 hardware the MP outcome it would need is forbidden anyway; the ARM MP results above are the repository's demonstration that relaxed message passing can fail.

**Data race (lost updates)**
- Runnable: [`Task-9/level1_Basics/raceCondition.cpp`](../Task-9/level1_Basics/raceCondition.cpp), 4 threads × 1,000,000 increments of a plain `long long`. The committed screenshot (Windows PowerShell, `g++ -std=c++26`, no `-O`) shows **1,078,036** against an expected 4,000,000 (one run).
- Build flags matter. In a check on a 1-vCPU Linux sandbox, an `-O0` build lost updates (1,481,708 of 4,000,000), while an `-O2` build printed 4,000,000 because the compiler loads `counter` once, adds, and stores once per thread. A plain `long long` is formally undefined behaviour; the program demonstrates it, not a guarantee.
- Listings only: [`Task-10` report](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §9.1 (`race_demo.cpp`, 4 threads × 1,000,000 increments of a `volatile uint64_t`; totals over five runs 1,114,890; 1,362,566; 2,036,062; 1,570,796; 956,473) and [`task-8/research.md`](../task-8/research.md) (2 threads × 1,000,000 increments of a plain `int`; 1000000, 195582, 173522 against an expected 2,000,000). No standalone `race_demo.cpp` or saved run output is in the repository.

### 5.2 False Sharing Benchmark

Source: [`Task-7/false-sharing.cpp`](../Task-7/false-sharing.cpp) (Linux, C++20). Results: [`Task-7/measured-results/`](../Task-7/measured-results/). Write-up: [`Task-7/README.md`](../Task-7/README.md), [`false-sharing-results.md`](../Task-7/false-sharing-results.md).

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

Graph: [`false-sharing-graph.png`](../Task-7/measured-results/false-sharing-graph.png). Disassembly confirms the hot loop in all modes is `lock addq $0x1,(%rdx)` ([`hot-loop-assembly.txt`](../Task-7/measured-results/hot-loop-assembly.txt)).

**Limitations (from the authors' own notes and the environment record):** cloud KVM VM (Xeon Platinum 8573C, 9 vCPUs, cgroup quota of 8 CPUs); 38 of 105 runs saw cgroup throttling; the 9-thread row is over quota and spills packed counters onto a second line; `perf` was unavailable so cache-line transfers are inferred from the layout change, not counted; no bare-metal rerun yet.

A second, independent false-sharing figure appears in the Task-10 report (see [§6](#6-experiments--results)); its source and CSV are not in the repository.

### 5.3 Lock-Free Construct

All of the following are in [`Task-9/`](../Task-9/).

- **CAS retry loop** — [`casCounter.cpp`](../Task-9/casCounter.cpp): `compare_exchange_weak` (`seq_cst` / relaxed on failure) on a shared `long long`, with a per-thread retry count. Committed rows in [`cas-results.csv`](../Task-9/cas-results.csv): 8 threads × 1,000,000 → counter 8,000,000, 17,868,468 retries, 917 ms; 15 threads × 1,000,000 → 15,000,000, 41,971,194 retries, 2,055 ms. Both rows carry `RunID` 1 and the note "Manually run on my hardware"; hardware and flags are not recorded. A screenshot shows a different 8-thread run (18,312,541 retries, 922 ms) built with `g++ -std=c++26` and no `-O`. There is no `fetch_add` baseline.
- **Bounded SPSC ring buffer** — [`lockFreeRingBuffer.hpp`](../Task-9/lockFreeRingBuffer.hpp): `std::atomic` head and tail, release store / acquire load, no mutex, indices aligned to 64 bytes. The `Task-12` audit found the release/acquire protocol correct by inspection. There is no test that checks order or content (the benchmark only accumulates a `sink`), and no ThreadSanitizer run. The same design is listed in the Task-10 report (§9.4), whose source files are not in the repository.
- **Benchmark** — [`ringBufferBench.cpp`](../Task-9/ringBufferBench.cpp): 1,000,000 push + pop per queue, queue size 1,024, mutex + condition variable vs the ring buffer. Committed CSV: mutex 386 ms, ring buffer 35 ms (5,181,347 vs 57,142,857 ops/s counting each push and each pop, an 11.02× difference). One run, millisecond timing, no hardware record. The screenshot in `assets/` shows 351 ms vs 8 ms, an earlier build.
- **ABA** — [`abaDemo.cpp`](../Task-9/abaDemo.cpp): a naive Treiber stack and a tagged-pointer (16-bit tag in the upper bits) stack are defined; only the tagged stack is run (4 threads × 100,000 push/pop). The naive stack is never run, so ABA is never triggered or shown. The audit also points out that `pop` reads `old_top.ptr()->next` while another thread may already have deleted that node.
- **Not found:** the MPMC ring buffer behind the livelock story in `lockFree.md`; hazard-pointer, epoch or RCU code; a deterministic ABA reproducer; any data behind the "ABA overhead" table (tagged 22%, hazard 94%, RCU 15%) or the "cache misses 12.4M → 3.1M" table, both of which the audit marks unverified.
- Related but not CAS loops: [`task-8/locking-benchmark.cpp`](../task-8/locking-benchmark.cpp) benchmarks a `std::atomic<int>` increment against a mutex and an `atomic_flag` spinlock; Task-7 uses `fetch_add`.

### 5.4 Team Knowledge Transfer

- [`Task-12/kt-plan.md`](../Task-12/kt-plan.md): ten proposed sessions (orientation; atomics and data races; the memory orders; compiler vs hardware ordering; store buffers and fences; coherence; running the PoCs; benchmark methodology; lock-free correctness; evidence review), each with prerequisites, a proposed presenter, a demonstration, an expected outcome and a verification check. The file itself says its status is **proposed** and "Sessions held: None".
- [`Task-12/evidence-matrix.md`](../Task-12/evidence-matrix.md) and [`team-contributions.md`](../Task-12/team-contributions.md) serve as the traceability record for what each claim and member rests on.
- [`Task-10` report](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) §14 contains a 12-question walkthrough checklist and a 30-second explanation for Member 10.
- [`Task-4/REVIEW-AND-CORRECTIONS.md`](../Task-4/REVIEW-AND-CORRECTIONS.md) records a review of Member 4's deliverable and leaves a project-specific sign-off explicitly open.
- [`Task-7/README.md`](../Task-7/README.md) and [`false-sharing-results.md`](../Task-7/false-sharing-results.md) explain method, results and limits in the author's words.
- No walkthrough records, Q&A logs or sign-offs showing completed KT for any member.
- **Out-of-date parts of `Task-12`.** The audit was done at commit `415b759`, before the ARM update to `Task5` and before `task_3` existed. Its rows E07 and E08 (the old x86 `Task5` program and its unrun fence control) no longer apply to anything in the repository. E04 (no hardware demonstration of a relaxed-ordering failure) and E15 (ARM weak ordering with no ARM run) are now answered by the `Task5` MP and SB results, though a primary Arm architecture manual is still not cited. Its ledger rows for Members 3 and 5, and sessions 5 and 7 of the KT plan (which refer to an x86 `Task5` program and an `mfence` control), predate the current folders. The audit's own action 8 (update this README) is what this revision does. Its ledger row for Khushi Kumari ("Not documented / None found") is also stale now that `Task1` is present.

---

## 6. Experiments & Results

| Experiment | Hypothesis | Implementation | Measurement | Result |
|---|---|---|---|---|
| ARM litmus tests (Task5) | AArch64 reorders SB and MP when relaxed; `stlr`/`ldar` and `dmb` forbid it | [`Task5-ARM_AArch-Memory-Model/arm-litmus.cpp`](../Task5-ARM_AArch-Memory-Model/arm-litmus.cpp), runnable on AArch64 | Weak-outcome count over 2,000,000 runs per variant, 10 variants × 3 pinned CPU pairs | SB relaxed 11.9–21.4%; MP relaxed 0.57–2.56%; MP writer-release-only 0.0006–2.15%; every fully ordered variant and both LB variants 0; raw data and figures committed; one run per cell |
| Store buffering, paired (task_3) | Relaxed allows both loads to return 0, `seq_cst` does not | [`task_3/practical/store_buffering.cpp`](../task_3/practical/store_buffering.cpp), runnable (C++20) | `r1==0 && r2==0` count, barrier-paired rounds | No result committed (placeholder file) |
| Store buffering, free-running (Task-4) | x86-64 allows a load to pass an older store to a different address | [`Task-4/store-buffering-stress-test.cpp`](../Task-4/store-buffering-stress-test.cpp), runnable | `r0==0 && r1==0` count over 200,000 samples | Reported 31,822 (~16%); output not committed; harness questioned by the audit |
| Race PoC (lost updates) | Unsynchronised increments lose updates | [`Task-9/level1_Basics/raceCondition.cpp`](../Task-9/level1_Basics/raceCondition.cpp), runnable; listings in Task-10 PDF §9.1 and `task-8/research.md` | 4 threads × 1M, one screenshot run; 4 × 1M (PDF, 5 runs); 2 × 1M (notes, 3 runs) | 1,078,036 of 4,000,000 (screenshot, no `-O`); all recorded totals below expected; result depends on optimisation level |
| Release/acquire vs relaxed | Relaxed gives no synchronisation with `data` | [`Task-2/experiment4.cpp`](../Task-2/experiment4.cpp), [`4b`](../Task-2/experiment4b.cpp) | Printed value of `data` | Both print 42; no difference shown (the PDF says so) |
| False sharing (Task-7) | Padding to 64 B removes cache-line bouncing | [`Task-7/false-sharing.cpp`](../Task-7/false-sharing.cpp) | Median Mops/s, 7 runs, 1–9 threads, pinned | Padded/packed 3.50× at 4 threads, 9.62× at 8; 0.96× and 0.89× at 1 and 2; packed ≈ true shared at 8 (43.4 vs 43.8) |
| False sharing (Task-10, reported) | Same | Not in repo (`phase2_bench.cpp`) | 2 threads, median of 3 runs of 1 s | Reported 241.296 vs 1,040.680 Mops/s (≈4.31×), AMD EPYC 9V74 VM, no pinning |
| Mutex / spinlock / atomic | Contention hurts spinlocks most | [`task-8/locking-benchmark.cpp`](../task-8/locking-benchmark.cpp) | Mean time (ms) of 4 runs, 1M increments/thread, 1–8 threads | At 8 threads: mutex 558.75, spinlock 1572.00, atomic 119.50 ms (see data caveat below) |
| CAS retry loop | Retries grow with contention | [`Task-9/casCounter.cpp`](../Task-9/casCounter.cpp), runnable | Final count, retries, time | 8 threads: 8,000,000 correct, 17,868,468 retries, 917 ms; 15 threads: 15,000,000 correct, 41,971,194 retries, 2,055 ms; hardware unrecorded, one run each |
| SPSC ring buffer vs mutex queue | Lock-free SPSC has less overhead | [`Task-9/ringBufferBench.cpp`](../Task-9/ringBufferBench.cpp), [`lockFreeRingBuffer.hpp`](../Task-9/lockFreeRingBuffer.hpp) | 1M push+pop each, ms timing, one run | 386 ms vs 35 ms (≈11.02×); no correctness test; hardware unrecorded |
| ABA | Tagged pointers prevent ABA | [`Task-9/abaDemo.cpp`](../Task-9/abaDemo.cpp) | Tagged stack, 4 × 100,000 push/pop | Completes without a crash; naive stack never run, ABA never triggered |
| Ring buffer (Task-10, reported) | SPSC ring has less overhead than a mutex queue | Listing only: Task-10 PDF §9.4 | 5M messages, 3 runs, checksum | Reported medians 12.055 vs 84.163 Mops/s (≈6.98×); checksums matched; no source or CSV in repo |

**Store buffering on two architectures.** The ARM litmus rates (SB relaxed 11.9–21.4%) come from barrier-started, paired rounds on a phone. The x86 `Task-4` rate (~16%) comes from a free-running sampler on an unrecorded VM and is questioned by the audit. The harnesses and machines differ, so the two sets of numbers should not be compared with each other. The repository's earlier paired x86 litmus program (0.0086% in 1,000,000 runs) is no longer in `Task5`.

**Pairs differ.** In `Task5` the same code gives SB relaxed 11.9% on CPUs 0 & 1 and 21.4% on CPUs 6 & 7, and MP writer-release-only 43,049 hits on 6 & 7 against 79 and 11 elsewhere. The harness is also asymmetric (one thread usually starts first), so rates are specific to this device and harness.

**Data caveat for `task-8`.** [`result.csv`](../task-8/result.csv) has 42 of the 48 expected rows (run 3 at 1 thread and run 2 at 8 threads are absent). Averages in [`analysis.md`](../task-8/analysis.md) for 2 and 4 threads match the CSV. The 1-thread and 8-thread averages do not: for example, at 8 threads the CSV gives mutex 567.33, spinlock 1666.33, atomic 122.33 ms (3 runs each) against 558.75, 1572.00, 119.50 in the analysis. The ordering (atomic < mutex < spinlock at 8 threads) holds either way.

**Data caveat for `Task-9`.** `cas-results.csv` rows both have `RunID` 1. `ringBufferBench.csv` is overwritten on every run and `casCounter` appends a row, so the committed rows are whichever runs the author kept. `readme.md` and `lockFree.md` also quote a cache-miss table and an ABA-overhead table with no accompanying data.

---

## 7. Key Findings

### Experimentally demonstrated
Backed by code and data in this repository.

- **On a real AArch64 chip, relaxed store buffering and relaxed message passing both produce the weak outcome.** SB relaxed: 11.9%, 21.4% and 17.9% of 2,000,000 runs on the three CPU pairs. MP relaxed: 0.57%, 2.56% and 0.66% ([`Task5`](../Task5-ARM_AArch-Memory-Model/litmus_results/summary.txt)). MP is the case x86-64 cannot show, so this is the repository's demonstration that relaxed ordering can expose stale data.
- **Ordering removes it, in these runs.** `stlr`/`ldar` (release/acquire and `seq_cst`), `dmb ish` between store and load, and `dmb` / `dmb ishld` fences around the MP stores and loads all gave zero weak outcomes in 2,000,000 runs on every pair.
- **Half-ordered code still leaks.** MP with a release store but a plain reader gave 43,049 weak outcomes (2.15%) on CPUs 6 & 7, and 79 and 11 on the other two pairs.
- **Load buffering was not seen** on this chip even with relaxed ordering. The architecture permits it, so that zero is "not observed here".
- **The compiler can change a demo's outcome.** `Task-9`'s lost-update program loses updates at `-O0` and, in a sandbox check, not at `-O2` where the loop collapses to one load, one add and one store per thread. `Task-9` also reports a 43.87× benchmark that turned into 11.02× after the consumer's value was consumed.
- With threads pinned and work fixed, per-thread counters on one 64-byte line scale badly while one-counter-per-line scales: **9.62× higher median throughput at 8 threads, 3.50× at 4** ([`Task-7`](../Task-7/measured-results/summary.csv)). On this VM, packed counters performed about the same as a single truly shared counter at 8 threads.
- Padding gave no benefit at 1 thread and a median ~10% *loss* at 2 threads (wide, overlapping ranges; cause not established). Padded throughput at 8 threads was 3.2× its 1-thread value, not 8×.
- GCC (x86-64, `-O2`) emits plain `MOV` for relaxed/acquire loads and relaxed/release stores, `XCHG` for a `seq_cst` store and `LOCK XADD` for `fetch_add` ([`Task-4`](../Task-4/x86-atomics-gcc14-x86_64.s)). The AArch64 counterparts are in [`Task5`](../Task5-ARM_AArch-Memory-Model/arm-atomics.s): `ldr`/`str`, `ldar`/`stlr` and `dmb` variants inline, with read-modify-write operations compiled to outline-atomics helper calls (`__aarch64_ldadd4_acq_rel`, `__aarch64_cas4_acq_rel`, `__aarch64_swp4_acq_rel`); the `-march=armv8.1-a` build in [`arm-atomics-lse.s`](../Task5-ARM_AArch-Memory-Model/arm-atomics-lse.s) uses inline LSE instructions (`ldaddal`, `swpal`, `casal`). These are compiler observations, not architectural guarantees.
- `-O2` can remove work or branches before the CPU sees them (constant folding, `cmov`) ([`Task-2`](../Task-2/)).
- In the task-8 benchmark (Windows, i5-13500H), atomic increment was fastest and the spinlock slowest at 8 threads.
- The CAS counter is correct (final counts equal expected) and its retries are large: about 2.2 retries per increment at 8 threads and 2.8 at 15 (computed from the two committed rows; different hardware state and thread counts, one run each).
- The SPSC ring buffer finished a 1,000,000-item transfer far faster than the mutex queue in the committed run (35 ms vs 386 ms). The direction is plausible; the size of the gap is environment-dependent and was measured once.

### Research-supported
Supported by cited sources, not experimentally demonstrated here.

- x86-64 is TSO: store→load reordering is allowed, other reorderings are not; `LFENCE` serialises instruction execution rather than ordering memory visibility ([`Task-4`](../Task-4/x86-memory-model.md)).
- Release/acquire creates a happens-before relationship; relaxed does not ([`Task-2`](../Task-2/notes.md), [`task_3`](../task_3/theory/theoretical-background.md), `Task-4`).
- ARM/AArch64 is weakly ordered, allowing all four load/store reorderings, and is multicopy atomic since ARMv8; `DMB`, `LDAR`/`STLR` and the LSE atomics provide ordering ([`Task5`](../Task5-ARM_AArch-Memory-Model/README.md), citing Pulte et al., POPL 2018, and Arm Learning Paths). The Arm Architecture Reference Manual itself is not cited.
- MESI has four states; MOESI adds Owned; reported gains of MOESI and variants come from specific simulation studies and should not be generalised ([`Task - 6.pdf`](../Task%20-%206.pdf)).
- Compiler optimisation, out-of-order execution and memory ordering are separate mechanisms ([`Task-2`](../Task-2/cpu-reordering.pdf)).
- Spinlocks burn CPU while waiting; mutexes block and pay scheduling cost ([`task-8/research.md`](../task-8/research.md)).
- The ABA problem and its remedies (tagged pointers, hazard pointers, RCU) are described with references in [`Task-9/abaReferences.md`](../Task-9/abaReferences.md); no measurement here supports the overhead percentages in `abaBenchMarkAnalysis.md`.
- The Disruptor combines a preallocated ring with sequences, barriers and consumer dependency graphs, and is not claimed to be always faster than queues ([`Task-10`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf)).

### Still unverified
Needs additional experiments or files.

- That x86-64 hardware shows store buffering under a valid harness: `Task-4`'s is questioned, `task_3` has no result, and the earlier paired x86 program is no longer in the repository.
- Which CPU numbers are Cortex-A55 and which Cortex-A78 in the `Task5` pairs, whether pinning succeeded in every run, repeat-run variation, and any effect of frequency or temperature.
- That ARM litmus results match the formal model (no rmem or herd7 run), and any LB outcome on any ARM chip.
- The SPSC ring buffer's correctness under test (no check of order or content, no TSan) and the size of its advantage over a mutex queue (one run here; ≈6.98× reported separately in Task-10).
- The ABA problem itself (the naive stack is never run), safe reclamation after `pop` (the demo deletes nodes that another thread may still be reading), the ABA overhead table, the cache-miss table, and the MPMC livelock story (no code).
- The data-race lost-update counts in the Task-10 and `task-8` listings (no source file or saved output).
- The ≈4.31× false-sharing result on AMD EPYC and why it differs from Task-7's 0.89× at 2 threads (different hardware, thread pinning and run length; not investigated).
- That cache-line transfers cause the false-sharing slowdown (`perf c2c` not run; the mechanism is inferred from layout changes only).
- Any relaxed-ordering failure on x86 (none can occur for MP there), CAS contention against a `fetch_add` baseline, MPSC/MPMC.

---

## 8. Repository Structure

```text
Hardware-Concurrency-Memory-Models-Lock-Free-Design/
├── README.md                         # this file
├── Task - 6.pdf                      # MESI/MOESI literature review (7 pages)
├── Task1/                            # Research foundations (2 .docx, concept map, 4 demo programs)
│   ├── Hardware_Concurrency_Research1.docx, Hardware_Concurrency_References.docx
│   ├── Concurrency_Concept_Map.png
│   └── 01_memory_hierarchy.cpp, 02_false_sharing.cpp, 03_memory_ordering.cpp, 04_lock_free.cpp
├── Task-2/                           # CPU reordering, ILP, compiler vs CPU, release/acquire
│   ├── notes.md, cpu-reordering.pdf  # notes and 25-page write-up
│   ├── add.cpp, add2.cpp, main.cpp   # early assembly-inspection programs
│   ├── experiment1–3.cpp / .md       # compiler optimisation, dependencies, branches
│   ├── experiment4.cpp, experiment4b.cpp, main2.cpp   # release/acquire vs relaxed
│   ├── *.s                           # generated -O0/-O2 assembly
│   └── *.exe                         # committed Windows binaries (add, add2, main2, experiment4, experiment4b)
├── task_3/                           # Store buffers and memory ordering (note lower-case folder with underscore)
│   ├── README.md                     # links a diagrams/ folder that does not exist
│   ├── theory/theoretical-background.md
│   ├── practical/store_buffering.cpp
│   ├── experiment/results.md         # placeholder, no measured output
│   ├── case-study/case-study.md
│   └── references/references.md
├── Task-4/                           # x86-64 memory model
│   ├── x86-memory-model.md, REAADME.md, REVIEW-AND-CORRECTIONS.md
│   ├── x86-atomics.cpp, x86-atomics-gcc14-x86_64.s
│   ├── x86-memory-litmus-tests.cpp, store-buffering-stress-test.cpp
│   └── x86-64-memory-ordering-comparison-table.csv
├── Task5-ARM_AArch-Memory-Model/     # ARM/AArch64 memory model, measured on a phone
│   ├── README.md, arm_memory_model.md
│   ├── arm-litmus.cpp                # SB / MP / LB harness (10 variants)
│   ├── arm-atomics.cpp, arm-atomics.s, arm-atomics-lse.s   # AArch64 assembly (ARMv8.0 and LSE)
│   ├── plot_litmus.py
│   ├── litmus_results/               # summary.txt + cpu0-1/, cpu6-7/, cpu0-7/ (10 variant folders each, 2 .txt files per folder)
│   └── plots/                        # 8 PNG figures
├── Task-7/                           # False-sharing benchmark
│   ├── false-sharing.cpp, run-benchmark.sh, analyze.py, requirements.txt, .gitignore
│   ├── README.md, false-sharing-results.md, RESEARCH.md
│   └── measured-results/             # raw.csv, summary.csv, graph, environment.txt, benchmark.log, hot-loop-assembly.txt
├── task-8/                           # Mutex / spinlock / atomic contention (note lower-case folder name)
│   ├── locking-benchmark.cpp, locking-benchmark.exe
│   ├── result.csv, graph.png, raw_data.png
│   └── analysis.md, research.md
├── Task-9/                           # CAS and lock-free algorithms
│   ├── readme.md, lockFree.md
│   ├── casCounter.cpp, cas-results.csv
│   ├── lockFreeRingBuffer.hpp, ringBufferBench.cpp, ringBufferBench.csv
│   ├── abaDemo.cpp, abaBenchMarkAnalysis.md, abaReferences.md, abaTextImage.txt
│   ├── assets/                       # 3 screenshots (CAS, ring buffer, ABA)
│   ├── level1_Basics/                # raceCondition.cpp, sharedMemory.cpp, sharedMemory.txt, Images/ (2 screenshots)
│   └── .gitignore
├── Task-10/
│   └── Ring_Buffer_LMAX_Disruptor_Final_Report.pdf
├── Task-11/                          # Performance-engineering methodology
│   ├── README.md, benchmark-template.csv (header only)
│   ├── methodology/benchmark-methodology.md
│   └── research/cpu-and-hardware-concurrency.md
└── Task-12/                          # Evidence audit, bibliography, ledger, KT plan
    ├── evidence-matrix.md, sources.md
    └── team-contributions.md, kt-plan.md
```

There is no top-level build system, test directory or CI configuration. Folder names are inconsistent: `Task1` (no hyphen), `Task5-ARM_AArch-Memory-Model` (not `Task-5`), `task_3` and `task-8` (lower case), `Task - 6.pdf` (a file, with spaces).

---

## 9. Team Contributions

Folders and commit messages use member numbers (`member-2`, `member-4`, "Member 11", "Member 10 — Ananya Narula" in the PDF, "Member 12" in `Task-12`). Numbers 1–12 follow the roster order below. The member-to-folder link is explicit for Members 2, 4, 10, 11 and 12 and inferred from roster order plus commit author for Members 3, 5, 6, 7, 8 and 9, and from folder number and roster order alone for Member 1 (`Task1`; the snapshot used for the second audit pass has no commit data). Task 5's commits came from the handle `Glazybyte`, which matches no roster name, so that attribution rests on the folder number alone. The commit author name or GitHub handle is given where it differs from the roster name.

| Member | Phase 1 Area | Phase 2 Work | Key Artifact |
|---|---|---|---|
| Khushi Kumari (inferred from `Task1`) | CPU–cache–RAM hierarchy, concurrency vs parallelism, false sharing, memory-ordering and lock-free research note and reference list | Four demo programs and a concept map (no results committed) | [`Task1/`](../Task1/) |
| Pushparaj Singh | CPU execution, compiler vs OoO, memory-ordering notes (`Task-2`). | Release/acquire vs relaxed example (source only). Repository owner; committed the Task-10 and Task-12 files. | [`Task-2/cpu-reordering.pdf`](../Task-2/cpu-reordering.pdf), [`notes.md`](../Task-2/notes.md) |
| Anand Kumar Sahni | Store buffers, reordering, visibility, memory orders and fences (theory, case study, references) | Paired store-buffering experiment in C++20 (no result recorded) | [`task_3/`](../task_3/), [`practical/store_buffering.cpp`](../task_3/practical/store_buffering.cpp) |
| Anshul Prajapati (`Anshul036`) | x86-64 memory model, fences, litmus tests, C++→assembly mapping | Store-buffering stress test | [`Task-4/x86-memory-model.md`](../Task-4/x86-memory-model.md), [`store-buffering-stress-test.cpp`](../Task-4/store-buffering-stress-test.cpp) |
| Devansh Vinayak (inferred; committed as `Glazybyte`) | Memory-model concepts, ARM/AArch64 ordering instructions, C++→AArch64 assembly (plain and LSE) | ARM litmus-test suite (SB, MP, LB; 10 variants) run on an Android phone across three CPU pairs, raw data, 8 plots | [`Task5-ARM_AArch-Memory-Model/README.md`](../Task5-ARM_AArch-Memory-Model/README.md), [`arm-litmus.cpp`](../Task5-ARM_AArch-Memory-Model/arm-litmus.cpp), [`litmus_results/`](../Task5-ARM_AArch-Memory-Model/litmus_results/) |
| Gagan Chaurasia | MESI/MOESI cache coherence review (uploaded the PDF; folder number inferred) | Not verifiable from repository. | [`Task - 6.pdf`](../Task%20-%206.pdf) |
| Harsh Gupta (`Harsh`) | False sharing and cache-line bouncing research | False-sharing benchmark, analysis script, raw data, graph | [`Task-7/`](../Task-7/) |
| Paras Gupta (`paras gupta`, `Astro-peek`) | Mutex, spinlock, contention research | Mutex/spinlock/atomic contention benchmark (Windows) | [`task-8/analysis.md`](../task-8/analysis.md), [`locking-benchmark.cpp`](../task-8/locking-benchmark.cpp) |
| Preksha Wani | CAS, retry loops, ABA and reclamation references, lock-free progress | CAS counter, SPSC ring buffer and benchmark, tagged-pointer stack, race and shared-memory demos, with screenshots | [`Task-9/`](../Task-9/), [`lockFreeRingBuffer.hpp`](../Task-9/lockFreeRingBuffer.hpp), [`casCounter.cpp`](../Task-9/casCounter.cpp) |
| Ananya Narula | Ring buffer, SPSC/MPSC/MPMC, LMAX Disruptor (PDF names the author; committed by Pushparaj) | Race demo, false-sharing and SPSC ring-buffer listings with reported results; KT checklist | [`Task-10/…Final_Report.pdf`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) |
| Chhavi Sharma | Performance-engineering methodology, hardware-concurrency introduction | Benchmark template (header only); no benchmark data collected | [`Task-11/methodology/benchmark-methodology.md`](../Task-11/methodology/benchmark-methodology.md) |
| Richa Bharti (`richa123-bharti`) | Evidence, documentation and knowledge transfer (self-stated; files headed "Member 12") | 30-claim evidence matrix, 24-entry source log, contribution ledger, proposed KT plan; committed by Pushparaj | [`Task-12/evidence-matrix.md`](../Task-12/evidence-matrix.md), [`kt-plan.md`](../Task-12/kt-plan.md), [`team-contributions.md`](../Task-12/team-contributions.md) |

All twelve roster slots map to a folder or file. Attributions that rest on inference: Khushi Kumari's identity for `Task1` (folder number and roster order only), Anand Kumar Sahni's folder number (`task_3`), Devansh Vinayak's identity (`Glazybyte`), Gagan Chaurasia's folder number, and the Task-10 and Task-12 authorship (stated in the files, committed by another member). The `Task-12` ledger was written before `task_3` and the ARM update existed and still lists Anand Kumar Sahni and Khushi Kumari as having no artifact.

---

## 10. How to Run

The build and run commands for Tasks 2, 4, 7 and 8 were executed in an earlier pass on a scratch copy of the repository on Ubuntu 24.04 with g++ 13.3.0 and Python 3, and compiled and ran as described. For this revision, on the same toolchain on a **1-vCPU sandbox**, the `task_3` and `Task-9` programs were compiled and run, and `plot_litmus.py` was run on the committed `Task5` data. The `Task5` measurements need an AArch64 device and were not repeated. Timing and count results from a 1-vCPU machine only check that the programs work; they say nothing about cross-core behaviour and are not committed evidence.

```bash
git clone https://github.com/pushparajwastaken/Hardware-Concurrency-Memory-Models-Lock-Free-Design.git
cd Hardware-Concurrency-Memory-Models-Lock-Free-Design
```

### Requirements
- Linux or WSL2 for `Task-7` (uses `sched_setaffinity`, `/sys`, `taskset`, `lscpu`, `ldd`); GCC 11+ with C++20.
- An AArch64 Android phone with Termux and `clang` for the `Task5` measurements; at least 2 CPUs, ideally 8 with known clusters. Python 3 with numpy and matplotlib for `plot_litmus.py` (works on any machine).
- C++20 (`std::barrier`) for `task_3`. Any g++ with `-pthread` for the others. No JDK, no Java, no other dependencies. `matplotlib` only for `Task-7/analyze.py` (pinned to 3.10.8 in `requirements.txt`).

### Building the `Task1` demo programs

```bash
cd Task1
for f in 01_memory_hierarchy 02_false_sharing 03_memory_ordering 04_lock_free; do g++ -O2 -std=c++17 -pthread $f.cpp -o $f; done
```

Re-check: all four compiled with g++ 13.3.0; they were not run. No build commands are documented in the folder. `02_false_sharing.cpp` uses 100,000,000 iterations, so expect a long run on a slow machine.

### Running the ARM litmus tests (`Task5`, on an AArch64 device)

```bash
# in Termux
pkg install clang
cp Task5-ARM_AArch-Memory-Model/arm-litmus.cpp ~/ && cd ~
clang++ -O2 -std=c++17 -pthread arm-litmus.cpp -o litmus

./litmus 2000000                 # one unpinned run of every test
./litmus 2000000 0:1 2:5 6:7     # one run per CPU pair (thread 0 on the first CPU, thread 1 on the second)
./litmus 2000000 auto            # pick three pairs from the CPUs' max frequencies
./litmus 2000000 auto run2       # same, writing into ./run2 instead of ./results
```

Build and run from Termux's home directory; shared storage like `Download` is mounted non-executable. Each run writes `summary.txt` and one folder per pair and variant. Then, on any machine:

```bash
cd Task5-ARM_AArch-Memory-Model
pip install numpy matplotlib
python plot_litmus.py litmus_results --out plots_local     # table of weak-outcome percentages + 8 PNGs
```

On the committed data the script printed the same percentages as the README (for example SB relaxed 11.9253%, 17.8945%, 21.3815% for CPUs 0 & 1, 0 & 7, 6 & 7). `arm-litmus.cpp` also builds with `g++ -O2 -std=c++17 -pthread` on x86-64 (it uses `pause` instead of `yield` there), but on a 1-vCPU machine its first test took about 67 s for 100,000 iterations and found no weak outcome, so x86 runs of it say nothing about ARM. To read the assembly: `g++ -O2 -S arm-atomics.cpp` and the `-march=armv8.1-a` variant (see the Task5 README).

### Running the false-sharing benchmark (`Task-7`)

```bash
cd Task-7
bash run-benchmark.sh results-local 10000000 7 auto   # output dir, iterations/thread, repeats, thread counts (auto or e.g. 1,2,4,8)
pip install -r requirements.txt
python3 analyze.py results-local                      # writes summary.csv and a graph into results-local/
```

The script builds with `-O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic`, records the environment, and refuses to overwrite an existing `raw.csv`. If your cache line is not 64 bytes, run with `CACHE_LINE_BYTES=<size>`. Quick smoke test: `bash run-benchmark.sh results-test 200000 1 1,2` (re-check: this needs at least two allowed CPUs; on a 1-CPU machine it stops with "Thread count exceeds allowed CPUs", leaves an empty `raw.csv` in the output directory, and the script then refuses to reuse that directory. Use a fresh directory name, or `1` as the thread list, on a 1-CPU machine.)

### Running the paired store-buffering experiment (`task_3`)

```bash
cd task_3
g++ -std=c++20 -O2 -pthread practical/store_buffering.cpp -o store_buffering
./store_buffering 100000 relaxed
./store_buffering 100000 seq_cst
```

On the 1-vCPU sandbox both printed `Both-zero outcomes: 0`, as expected without a second core. Record your own output in [`experiment/results.md`](../task_3/experiment/results.md); on a multi-core machine pin the threads (for example with `taskset`) and note the CPU.

### Running the store-buffering stress test (`Task-4`)

```bash
cd Task-4
g++ -std=c++20 -O2 -pthread store-buffering-stress-test.cpp -o sb && ./sb
# prints: rounds sampled: 200000, store-buffering outcomes (r0==0 && r1==0): N
```

`N` varies by machine (the documented run reported 31,822; an earlier spot-check on a different VM gave 8,550). The `Task-12` audit shows that an all-`seq_cst` version of this program gives counts of the same order, so `N` does not isolate store buffering. Re-check on a 1-vCPU sandbox, where no cross-core store buffering can occur: **124,667 of 200,000** samples, which supports the audit's conclusion that the harness counts its own bookkeeping.

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

### Running the CAS, ring-buffer and race programs (`Task-9`)

No build commands are documented in `Task-9` (the screenshots show `g++ -std=c++26` with no `-O` on Windows). The following compiled and ran with g++ 13.3.0:

```bash
cd Task-9
g++ -O2 -std=c++17 -pthread casCounter.cpp -o casCounter && ./casCounter            # appends a row to cas-results.csv
g++ -O2 -std=c++17 -pthread ringBufferBench.cpp -o ringBufferBench && ./ringBufferBench   # overwrites ringBufferBench.csv
g++ -O2 -std=c++17 -pthread abaDemo.cpp -o abaDemo && ./abaDemo
g++ -O0 -std=c++17 -pthread level1_Basics/raceCondition.cpp -o raceCondition && ./raceCondition
g++ -O2 -std=c++17 -pthread level1_Basics/sharedMemory.cpp -o sharedMemory && ./sharedMemory
```

Run them in a copy, or restore the CSVs afterwards: `casCounter` appends to and `ringBufferBench` overwrites the committed data files. Use `-O0` for `raceCondition`; at `-O2` the loop collapses and the race rarely shows. On the 1-vCPU sandbox: `casCounter` finished with the correct total and 7 retries (88 ms), the ring-buffer benchmark gave 100 ms (mutex) vs 11 ms (ring), `abaDemo` completed, and `raceCondition` at `-O0` printed 1,481,708 of 4,000,000. A later re-check on another 1-vCPU sandbox printed 4,000,000 of 4,000,000 (no lost updates), so the lost-update count depends on scheduling and core count as well as optimisation level.

### Running the locking benchmark (`task-8`)

No build command is documented in the repository (the committed `.exe` is a Windows binary; the analysis states MinGW-w64 GCC). This compiled and ran with:

```bash
cd task-8
g++ -O2 -std=c++17 -pthread locking-benchmark.cpp -o locking-benchmark && ./locking-benchmark
```

It prints one time (ms) per method for 1, 2, 4 and 8 threads. The committed results were produced on Windows 11, not Linux.

### Ring-buffer report sources

The Task-10 PDF (Appendix B) lists build commands for `phase2_bench.cpp` and `race_demo.cpp`, but neither file is present. The ring buffer in `Task-9` is the runnable equivalent.

---

## 11. Benchmark Methodology

What each dataset actually recorded:

| Item | Task-7 false sharing (measured) | task-8 locking (measured) | Task5 ARM litmus (measured) | Task-9 CAS and ring buffer (measured) | Task-10 report (reported only) | Task-4 stress test (reported only) |
|---|---|---|---|---|---|---|
| CPU | Xeon Platinum 8573C, KVM, 9 vCPUs (cgroup quota 8), 1 thread/core | i5-13500H, 2.60 GHz | MediaTek Dimensity 7400 Ultra: 4× Cortex-A78 @ 2.6 GHz + 4× Cortex-A55 @ 2.0 GHz; pairs 0 & 1, 6 & 7, 0 & 7 (cluster of each CPU number not recorded) | Not recorded ("my hardware"; screenshots show Windows PowerShell) | AMD EPYC 9V74, KVM, 5 CPUs | Not recorded ("x86-64 VM") |
| Cache | 64 B line (sysfs); L1d 240 KiB (5 instances), L2 10 MiB (5), L3 260 MiB | Not recorded | Not recorded | Not recorded | 64 B line | Not recorded |
| OS | Ubuntu 24.04.3, kernel 6.18.44 | Windows 11 | Android, Termux (version not recorded) | Windows (from screenshots) | Linux (x86_64) | Not recorded |
| Compiler / flags | g++ 13.3.0, `-O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic -DCACHE_LINE_BYTES=64` | MinGW-w64 GCC; version and flags not recorded | `clang++ -O2 -std=c++17 -pthread` (clang version not recorded) | Screenshots: `g++ -std=c++26`, no `-O` (CAS, race); ring-buffer build not shown | GCC 14.2, C++17, `-O2` | GCC 12.2.0, `-O2`, C++20 |
| Warm-up | 200,000 increments per thread per mode/count | None | None in the harness | None | One warm-up run | Not stated |
| Iterations | 10,000,000 per thread | 1,000,000 per thread | 2,000,000 runs per variant | CAS: 1,000,000 per thread; ring: 1,000,000 push + pop | 5,000,000 messages (ring); 1 s per layout (false sharing) | 200,000 sampling rounds |
| Repetitions | 7, shuffled order, fixed seed | 4 | 1 per variant and pair (30 test runs, plus one unpinned set) | 1 (CAS: 1 row per thread count) | 3 | 1 |
| Threads | 1, 2, 4, 8, 9, pinned to CPU *i* | 1, 2, 4, 8, not pinned | 2 per test, pinned to the pair (success not logged), spin-barrier start, random 0–127 nop offset | CAS: 8 and 15; ring: 1 producer + 1 consumer; not pinned | 2 (false sharing); 1 producer + 1 consumer; not pinned | 2 |
| Unit / clock | Mops/s; `steady_clock` | Milliseconds (integer); `high_resolution_clock` | Weak-outcome count and %, cumulative per 1,000 runs and per second; each test about 0.5–1.3 s | Milliseconds (integer); `high_resolution_clock` | Mops/s | Count of outcomes |
| Statistics | Median, min, max, Q1, Q3 | Mean only | Counts, rate, all four `(r0,r1)` outcome counts | Single values | Median | Single count |
| Raw data in repo | Yes ([`raw.csv`](../Task-7/measured-results/raw.csv)) | Partial (42/48 rows) | Yes ([`litmus_results/`](../Task5-ARM_AArch-Memory-Model/litmus_results/)); unpinned run only as a README transcript | CSVs with 1–2 rows; screenshots | No | No |

`task_3` is not in the table because it has no recorded run.

**Missing or inconsistent across the project:**
- Task-11 sets a standard (P50/P90/P95/P99, CPU frequency, temperature, utilisation, affinity, NUMA) that no committed dataset follows. `benchmark-template.csv` has no data rows.
- CPU frequency, temperature and utilisation were not recorded for any run, including the phone runs, where thermal and DVFS behaviour is likely to matter.
- `Task5` is the first dataset that records its CPU, OS and compiler flags, runs controls, and commits per-run raw data, but it has one run per cell, no warm-up, and no log of whether pinning succeeded.
- `task-8` used no warm-up, no pinning and millisecond resolution (1-thread atomic runs are 5–7 ms); compiler version, flags and thread-affinity are not recorded.
- `Task-9` timings are integer milliseconds from a single run, with hardware unrecorded; its ring-buffer test at 8–35 ms sits close to timer and scheduling noise.
- Task-7 recorded cgroup throttling per run (38 of 105 runs affected) and discloses it.

---

## 12. Reproducibility

- **Best-documented path:** `Task-7`. Environment, build command, binary hash, raw CSV, log and disassembly are committed; `run-benchmark.sh` regenerates everything and `analyze.py` re-derives the summary. Running `analyze.py` on the committed `raw.csv` reproduces the medians in the table in [§5.2](#52-false-sharing-benchmark).
- **Task5:** the plots and the percentage table regenerate from the committed raw files with `plot_litmus.py` (checked: same percentages as the README). The measurements themselves need an AArch64 device; the exact rates will differ by device, by CPU pair and by run. The qualitative result to expect: relaxed SB and MP show weak outcomes, release/acquire, `seq_cst` and `dmb` variants show none.
- **Expected output:** `Task-7` and `Task5` have committed results to compare against, and both depend on the machine. Qualitatively, packed throughput should fall with thread count while padded rises; absolute numbers will differ, especially on a VM with CPU quotas.
- **Run on bare metal** (or at least without a CPU quota) before drawing hardware conclusions about false sharing; the authors list `perf c2c record` as the next step. For `Task5`, record the cluster of each CPU number and repeat each pair several times.
- **Raw data locations:** [`Task-7/measured-results/`](../Task-7/measured-results/), [`Task5-ARM_AArch-Memory-Model/litmus_results/`](../Task5-ARM_AArch-Memory-Model/litmus_results/), [`task-8/result.csv`](../task-8/result.csv), [`Task-9/cas-results.csv`](../Task-9/cas-results.csv) and [`ringBufferBench.csv`](../Task-9/ringBufferBench.csv). None for Task-4 or Task-10 results, and none for `task_3`.
- **Graph generation:** `python3 analyze.py <folder>` in `Task-7`; `python plot_litmus.py litmus_results` in `Task5-ARM_AArch-Memory-Model/`. The `task-8` graphs are committed PNGs with no generating script; they plot the averages from `analysis.md`.
- **Known limits:** the only ARM hardware data come from one phone; committed `.exe` files are Windows binaries and are not needed to rebuild; Task-10 numbers cannot be reproduced because the source and CSVs are absent; the `Task-12` audit ran on a 1-vCPU VM and marks no claim as independently reproduced.

---

## 13. Limitations & Known Gaps

- **Phase 2 item 1:** the ARM litmus suite is runnable, has controls and commits raw data, but is one run per variant and pair on one phone. The lost-update program in `Task-9` has one recorded run and depends on `-O0`; the Task-10 and `task-8` versions are listings with unsaved output. `Task-4`'s store-buffering harness is questioned and has no committed raw output; `task_3`'s has no result.
- **Phase 2 item 3:** the CAS counter, SPSC ring buffer and tagged stack run, but there is no correctness test for any of them, the ABA hazard is never triggered, memory reclamation is not implemented, the MPMC design behind the livelock story is absent, and the Task-10 appendix lists files as "included" that are not in the repository.
- **Phase 2 item 4:** the KT plan is proposed and no session is recorded. KT evidence exists for Member 10 (a question list) and Member 4 (a review file) only.
- **ARM data:** one device, three CPU pairs, one run each, no warm-up, frequency and temperature unrecorded, pinning success unlogged, CPU-to-cluster mapping unrecorded (and, if the usual numbering applies, the "fast pair" and "slow pair" are big and little cores). LB never appears, so the architecture's permission for it is untested. No formal-model check.
- **x86 data:** no valid committed store-buffering measurement. The paired x86 litmus program formerly in `Task5` has been moved out of the repository, and its 86-reorders result is no longer backed by files here.
- **Benchmarks:** only Task-7 meets its own standard. `task-8` has an incomplete CSV and averages that disagree with it for 1 and 8 threads. `Task-9` has single-run, millisecond-resolution timings with unrecorded hardware. No dataset records P90/P95/P99, frequency or temperature.
- **Unsupported claims in `Task-9`:** the "75% cache-miss reduction" table, the ABA-overhead table (22% / 94% / 15%), the statement that the naive stack "crashes or livelocks", the MPMC livelock and its `yield()` fix, and the exact cause of the 43.87× result (the audit's sandbox did not reproduce a change of that size). The screenshot in `Task-9/assets/` shows the earlier build (351 ms vs 8 ms), not the 386 ms vs 35 ms in the CSV and `readme.md`.
- **Hardware evidence:** Task-7 ran on a throttled VM with no hardware counters. Task-10 ran on a different VM with unpinned threads. `Task-4` does not record its hardware. No bare-metal x86 data, no perf/HITM data.
- **Conflicting results:** padding helps 9.6× at 8 threads in Task-7 but did not help at 2 threads (0.89×); the Task-10 report claims 4.31× at 2 threads on other hardware. The ring-buffer advantage is 11.02× in Task-9 and ≈6.98× in Task-10 on different machines. No experiment reconciles these.
- **Experiment depth:** `Task-2` atomics experiment shows no failure for relaxed ordering (the ARM MP result is what supplies one); `Task-4` litmus tests (MP, IRIW) are definitions, not run harnesses.
- **Coverage:** `Task1` exists but has no README, no committed program output, and was not covered by the `Task-12` audit; some attributions are inferred (see [§9](#9-team-contributions)). Wait-free algorithms, hazard pointers, MPSC/MPMC and NUMA are not covered by any code.
- **Audit currency:** `Task-12` predates `task_3` and the ARM update to `Task5`; see [§5.4](#54-team-knowledge-transfer).
- **Document quality:** `Task-11/research/cpu-and-hardware-concurrency.md` is truncated; `Task-4/REAADME.md` has a misspelt file name; `Task-2/notes.md` contains Obsidian image links (`![[Pasted image …]]`) whose images are not in the repository; the Task-6 references are aggregator links (Consensus), some with truncated titles; `task_3/README.md` links a missing `diagrams/` folder; `Task-9/readme.md` begins with a blank title ("# — CAS / Lock-Free Algorithms"); `Task-9/lockFree.md` is written in the first person. The `Task5` assembly files were produced with GCC 13 while the measurements were compiled with clang; neither AArch64 compiler version is recorded beyond that.
- **Repository hygiene:** Windows `.exe` files are committed in `Task-2` and `task-8`; `Task-9/.gitignore` excludes `*.exe` but the CSVs are rewritten by running the programs.

---

## 14. Engineering Lessons

Each lesson is tied to the team's own work.

- **Source order is not the whole execution story.** `-O2` folded a call to a constant and replaced a branch with `cmov` ([`Task-2`](../Task-2/)); on AArch64 a relaxed store-then-load or write-then-write is observed out of order on 12–21% and 0.6–2.6% of runs ([`Task5`](../Task5-ARM_AArch-Memory-Model/)).
- **Both sides of a protocol need ordering.** Message passing with a release store and a plain reader still failed 2.15% of runs on one CPU pair; release on one side, or a fence on one side, is not enough ([`Task5`](../Task5-ARM_AArch-Memory-Model/README.md)).
- **One CPU pair is not the machine.** The same code produced 43,049 weak outcomes on one pair and 79 and 11 on the other two. Test the combinations of cores you care about, and record which core is which.
- **"Not observed" is not "forbidden".** Load buffering never appeared on this chip, yet the architecture allows it; likewise the x86 store-buffering stress test showed a number that turned out not to prove what it was meant to ([`Task-12`](../Task-12/evidence-matrix.md) E06). Use control runs (`seq_cst`, `dmb`), and formal tools where possible.
- **A harness can fail to discriminate.** A test that gives similar counts with and without the ordering under test measures its own bookkeeping. `Task5` includes the controls; `Task-4` did not.
- **A compiler barrier is not a CPU fence, and a demo can depend on the optimiser.** On AArch64, `asm volatile("" ::: "memory")` emits no instruction and only constrains the compiler. `raceCondition.cpp` loses updates at `-O0` and not at `-O2`; `Task-9`'s first ring-buffer figure came from a build whose consumer work did not count.
- **Memory ordering is not cache coherence.** Coherence keeps copies of one location consistent; ordering governs how operations on different locations become visible ([`Task-2/notes.md`](../Task-2/notes.md), [`task_3`](../task_3/theory/theoretical-background.md), `Task - 6.pdf`).
- **A C++ memory order is not an instruction.** Several orders compile to the same x86 instruction, so "it works on x86" says little about portability. On AArch64 they map to different instructions (`ldar`/`stlr`, `dmb`, LSE `*al` forms), and code that was safe on x86 by accident can break there ([`Task-4`](../Task-4/x86-memory-model.md), [`Task5`](../Task5-ARM_AArch-Memory-Model/arm-atomics.s)).
- **Atomic does not mean fast.** Atomic increments on one cache line performed no better than a truly shared counter at 8 threads ([`Task-7`](../Task-7/false-sharing-results.md)); a CAS loop at 15 threads retried about 2.8 times per increment ([`Task-9`](../Task-9/cas-results.csv)).
- **False sharing arises between different variables.** Each thread had its own counter, yet packed counters were up to ~9.6× slower than padded ones ([`Task-7`](../Task-7/)).
- **Padding is not free and does not always help.** No gain at 1–2 threads, and 8× the memory for 8 counters.
- **Contention costs depend on the primitive.** At 8 threads the spinlock was slowest and the atomic fastest in the task-8 workload ([`task-8`](../task-8/analysis.md)).
- **A lock-free structure still needs tests.** The SPSC ring buffer looks correct by inspection, but nothing checks its output; the "ABA-safe" demo never runs the unsafe version, so it cannot show the bug it fixes ([`Task-9`](../Task-9/), [`Task-12`](../Task-12/evidence-matrix.md) E20, E25).
- **Benchmark method matters.** Pinning, shuffled run order, runtime layout checks, correctness checks and disclosure of CPU throttling shaped how far Task-7 could be trusted; the unrecorded conditions in other datasets limit what they show ([`Task-11`](../Task-11/methodology/benchmark-methodology.md)).
- **Audit your own claims.** `Task-12` traced 30 claims and found only 6 adequately supported; writing down, per claim, what would falsify it is what turned several "results" into "to do" items.
- **Hardware observations need qualification.** VM, quota, compiler, CPU, optimisation level and core pair all appear in the authors' own caveats.
- **Not demonstrated by this repository:** "lock-free is faster" on more than one machine, CAS retry cost against a baseline, and ABA corruption. The Task-10 report states it does not claim that a ring buffer or the Disruptor is always faster.

---

## 15. References

Taken from the repository's documents; see each file for full citations, and [`Task-12/sources.md`](../Task-12/sources.md) for a 24-entry bibliography with a verification status for each source.

**Research foundations** ([`Task1/Hardware_Concurrency_References.docx`](../Task1/Hardware_Concurrency_References.docx))
- cppreference: `std::memory_order` and atomic operations; Linux kernel documentation, *False Sharing*; Intel 64 and IA-32 Software Developer Manuals; Arm Architecture Reference Manuals (listed here, unlike in `Task5`); LMAX Disruptor repository; Herb Sutter, *Lock-Free Code: A False Sense of Security*; ISO C++ working draft (data races)

**x86 and CPU architecture** ([`Task-4/x86-memory-model.md`](../Task-4/x86-memory-model.md))
- Intel® 64 Architecture Memory Ordering White Paper
- Owens et al., *A Better x86 Memory Model: x86-TSO* (2009)
- Intel SDM — `SFENCE`
- *The Semantics of x86 Multiprocessor Machine Code*

**ARM/AArch64 and weak memory models** ([`Task5`](../Task5-ARM_AArch-Memory-Model/README.md))
- Pulte, Flur, Deacon, French, Sarkar, Sewell, *Simplifying ARM Concurrency: Multicopy-Atomic Axiomatic and Operational Models for ARMv8* (POPL 2018)
- Preshing, *Weak vs. Strong Memory Models* and *Memory Reordering Caught in the Act*
- Arm Learning Paths, *Learn about the C++ memory model for porting applications to Arm*
- Zhang et al., *Weak Memory Models: Balancing Definitional Simplicity and Implementation Flexibility*
- Intel, *Intel 64 Architecture Memory Ordering White Paper* (x86 reordering rules)
- rmem, the formal ARM/POWER/RISC-V concurrency model tool (cited as the next step; not run)
- Not cited anywhere in `Task5`: the Arm Architecture Reference Manual (DDI 0487); `Task-12/sources.md` lists it as not yet checked.

**Store buffers and the C++ memory model** ([`task_3/references/references.md`](../task_3/references/references.md))
- cppreference: `std::memory_order`, `std::atomic`; the ISO C++ standard page; Intel SDM; Linux kernel documentation on memory barriers and false sharing; Preshing

**C++ memory model and atomics**
- C++ standard `[atomics.order]`; GCC `__atomic` Builtins documentation; LLVM *Atomic Instructions and Concurrency Guide* (`Task-4`)
- cppreference: `std::memory_order`, multi-threaded executions and data races, `std::mutex`, `std::atomic`, `std::atomic<T>::is_lock_free` ([`Task-10`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf))
- Boost.Atomic, *Thread coordination using Boost.Atomic* (`Task-4`)

**Cache coherence** ([`Task - 6.pdf`](../Task%20-%206.pdf))
- Alkhamisi 2022; Altwaijry & Alzahrani 2013; Amory & Ahmed 2021; Blanchet & Dupouy 2013; Dey & Nair 2014; Faeq & Omran 2021; Fotouhi et al. 2025; Ivanov & Nunna 2001; Kaur & Sulochana 2018; Kaushik et al. 2021; Kehagias & Raptis 2016; Komuravelli et al. 2014; Krishna & Rajeev 2025; Nair et al. 2021; Patil et al. 2019; Saraswat et al. 2021; Somarouthu 2025; Tiwari et al. 2014

**Locks, contention and blocking** ([`task-8/research.md`](../task-8/research.md))
- Raynal & Taubenfeld (2022); Anderson (1990); Dinh et al. (2018); Li, Ding & Shen (2007); Wieder & Brandenburg (2013); Federico, Marotta & Quaglia (2025)

**CAS, ABA and memory reclamation** ([`Task-9/abaReferences.md`](../Task-9/abaReferences.md))
- Michael & Scott, *Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms* (PODC 1996)
- Michael, *Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects* (IEEE TPDS 2004); Michael, *ABA Prevention Using Single-Word Instructions* (IBM RC23089, 2004)
- Harris, *A Pragmatic Implementation of Non-Blocking Linked Lists* (DISC 2001); the audit notes the description of this paper as "tagged-pointer" is unconfirmed
- SEI CERT CON09-C; C++ proposal P2530R3 (`std::hazard_pointer`); Intel SDM Vol. 3A §8.2

**Disruptor and ring buffers** ([`Task-10`](../Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf))
- LMAX Exchange: Disruptor technical paper, User Guide, Disruptor Wizard, source repository
- Intel® 64 and IA-32 Architectures Optimization Reference Manual (false sharing)

**False sharing and benchmarking** ([`Task-7/RESEARCH.md`](../Task-7/RESEARCH.md))
- Linux kernel documentation, *False Sharing*
- GCC documentation, *Built-in Functions for Memory Model Aware Atomic Operations*

---

## 16. Project Status

| Area | Status | Evidence |
|---|---|---|
| Phase 1 Research | 🟡 Partial | CPU execution, store buffers, x86-64, ARM/AArch64, MESI/MOESI, locks, CAS/ABA, ring buffer/Disruptor, methodology documented; `Task1` has research notes and demo programs but no committed output; no hazard-pointer or wait-free material; Task-6 and Task-11 have no code or data |
| ARM/AArch64 | ✅ Complete (with limits) | `Task5`: litmus harness, 10 variants × 3 CPU pairs × 2,000,000 runs, raw data, 8 figures, AArch64 assembly (plain and LSE), analysis and caveats; one device, one run per cell, no formal-model check |
| Race PoC | ✅ Complete | ARM litmus suite with controls (`Task5`), lost-update program (`Task-9`); `task_3` and `Task-4` harnesses without valid committed results |
| False Sharing | ✅ Complete | `Task-7`: source, script, raw CSV (105 runs), summary, graph, environment, disassembly; VM caveats disclosed |
| Lock-Free Construct | 🟡 Partial | `Task-9`: runnable CAS counter, SPSC ring buffer, tagged stack, with CSVs; no correctness test, no ABA reproducer, no reclamation, no MPMC code, several unsupported tables |
| Benchmarking | 🟡 Partial | `Task-7` complete; `Task5` raw data and environment recorded but single run per cell; `task-8` data incomplete and inconsistent with its analysis; `Task-9` single runs, hardware unrecorded; Task-10 numbers unverifiable; Task-11 standard not applied |
| KT | 🟡 Partial | Ten-session plan (proposed, none held); question checklist for Member 10; review notes for Member 4; no evidence for the rest |
| Evidence audit | 🟡 Partial | `Task-12`: 30 claims traced, 24 sources logged; done at `415b759`, before the ARM update and `task_3`, so some rows are stale; no claim independently reproduced |
| Final Integration | 🟡 Partial | This README indexes every folder; no consolidated report or cross-member results; `Task-11` template empty; `Task1` has no README or results |

---

## 17. Contribution / Team Note

This is a collaborative 12-member engineering and research project. Each member owned a numbered topic (`Task-N`), and the repository records their individual deliverables as submitted. Corrections to attribution or missing artifacts can be made by adding the files or contribution notes to the relevant `Task-N` folder.
