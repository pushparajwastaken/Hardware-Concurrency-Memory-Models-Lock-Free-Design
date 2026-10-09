# evidence-matrix.md — Claim-to-evidence traceability (Member 12)

Repo audited: `main` @ `415b759` (2026-10-09). Source IDs refer to `sources.md`.

## Status key
`VERIFIED — SOURCE` checked authoritative source · `VERIFIED — CODE` inspected implementation supports it · `EXECUTED` code was run and an execution record exists · `REPRODUCED` independently reproduced under documented conditions · `PARTIAL` · `UNVERIFIED` · `BLOCKED`.
**Auditor runs** (marked "AUD") were on one VM: 1 vCPU Intel Xeon @2.8 GHz, Linux 6.18.44, GCC 13.3.0. With one vCPU, no true cross-core behaviour (store buffering between cores, false sharing, CAS contention) can appear. AUD runs test harness logic and arithmetic only. No claim is marked REPRODUCED.

| ID | Technical claim | Source | Repo location | PoC / implementation | Test / benchmark | Observed result | Status | Gap / next action |
|---|---|---|---|---|---|---|---|---|
| E01 | `-O2` folds constants and replaces a branch with `cmov` (compiler, not CPU, removes work) | none cited (S11 is the nearest) | `Task-2/experiment1-3.*`, `*.s` | `experiment1.cpp`, `experiment3.cpp` | `-O0` vs `-O2` assembly | Assembly committed; I did not regenerate it | PARTIAL | Record compiler version and flags; cite a compiler doc |
| E02 | OoO execution cannot remove true RAW dependencies | none | `Task-2/experiment2.*` | `experiment2.cpp` | Assembly only | No timing measurement | UNVERIFIED | Add a timed dependent-vs-independent loop, or label as theory |
| E03 | Release/acquire creates happens-before; relaxed does not | S01, S02 | `Task-2/notes.md`, `Task-4/x86-memory-model.md` | `experiment4/4b.cpp`, `x86-memory-litmus-tests.cpp` | Prints `42` in both | Both print 42 (per README/PDF; I did not run) | PARTIAL | Source supports the claim. The PoC cannot fail on x86, and 4b reads non-atomic `data` without happens-before (UB). Do not cite as proof |
| E04 | Relaxed ordering can expose stale payload on hardware | S01 | — | none | none | No failing outcome produced | UNVERIFIED | Needs an ARM/Power run or a model checker; x86 forbids the MP outcome in hardware, so x86 cannot demonstrate it |
| E05 | x86 allows store→load reordering (SB outcome) | S05, S06, S03, S14 | `Task-4/x86-memory-model.md` | — | — | Literature | VERIFIED — SOURCE | Add SDM section number from the current SDM |
| E06 | `Task-4/store-buffering-stress-test.cpp` observes SB on hardware (31,822/200,000) | S05 (theory only) | `Task-4/store-buffering-stress-test.cpp` | same | Raw output not committed | AUD: original 26,852 / 112,775 / 122,412; **all-`seq_cst` control** 111,291 / 75,337 / 57,515 | UNVERIFIED | Harness cannot discriminate: the main thread's resets overwrite x,y so (0,0) is reachable under SC. Replace with a paired harness (see E07); keep the control. Retire the 16% figure |
| E07 | Paired SB litmus (Task5) observed 86 reorders in 1,000,000 runs | S14 (design source), S05 | `Task5…/src/ordering_modified.cpp`, `scripts/*.txt`, `images/output/` | same | 1M runs, CPUs 0,2, 200k warm-up | Per-second counts sum to 86; total 7.814 s. Order of magnitude consistent with S14's ~1/6,600 | EXECUTED | Not reproduced: AUD (1 vCPU) gave 0, as expected. Record CPU/OS/compiler. Rerun on ≥4 physical cores |
| E08 | A compiler barrier does not stop CPU reordering; `mfence` does | S14, S03 | `Task5…` (`USE_CPU_FENCE`) | same | Control **not run** | First half supported by E07; second half untested | PARTIAL | Run `USE_CPU_FENCE=1` and `USE_SINGLE_HW_THREAD=1`; commit output |
| E09 | MFENCE drains the store buffer | S03 (mirror, store-buffer section) | `Task-4/x86-memory-model.md`, `REAADME.md` | — | — | Repo gives no SDM citation | PARTIAL | Cite the current SDM section; keep "drains" as an Intel implementation statement separate from the architectural ordering guarantee; check AMD |
| E10 | LFENCE does not order memory visibility; it serializes instruction execution | S03 (partial) | `Task-4/x86-memory-model.md` | — | — | Not checked against current SDM | PARTIAL | Cite SDM LFENCE page and the Spectre-mitigation note |
| E11 | GCC emits MOV / XCHG / LOCK XADD / `lock or [rsp],0` for the listed orders | S11 | `Task-4/x86-atomics.cpp`, `x86-atomics-gcc14-x86_64.s` | same | `g++ -S -masm=intel` | Assembly committed (GCC 14.2); README says GCC 13.3 matched; I did not re-run | VERIFIED — CODE | Re-run and diff; treat as compiler observation, not architecture |
| E12 | MESI has M/E/S/I; MOESI adds Owned; snooping vs directory | S18 (not yet read), S22 | `Task - 6.pdf` | none | none | Literature review; text matches standard descriptions | PARTIAL | Cited papers unverified (aggregator links); no team state-transition diagram; check against S18 |
| E13 | Store buffers and invalidation queues explained and distinguished | S18, S03 | `Task-4/x86-memory-model.md` (store buffer only) | none | none | Store buffer: intuition only. **Invalidation queues: absent from the repo** | UNVERIFIED | Write a short note on invalidation queues and fences using S18; mark which statements are architectural vs implementation |
| E14 | Coherence does not replace ordering/synchronization | S18 | `Task-2/notes.md` (coherence section) | — | — | Conceptual | PARTIAL | Add an example (SB happens on coherent hardware) |
| E15 | AArch64 is weakly ordered; DMB/LDAR/STLR order accesses | S09, S10, S08 | `Task5…/README.md` Part 1 | none | no ARM run | External sources agree; repo cites none | PARTIAL | Cite S08; run MP/SB on ARM hardware or herd7 |
| E16 | Unsynchronized increments lose updates (data race) | S01 | `Task-9/level1_Basics/raceCondition.cpp` (+ screenshot, not viewed by me); listings in Task-10 PDF and `task-8/research.md` | same | counts: 5 runs (PDF), 3 runs (notes) | Totals below expected (reported) | PARTIAL | Task-10 `race_demo.cpp` absent. Commit one file with saved output; note the result is UB, not guaranteed |
| E17 | Padding to 64 B removes false-sharing slowdown (9.62× at 8 threads, 3.50× at 4) | S13 | `Task-7/` | `false-sharing.cpp` (SHA-256 `85b3fa…` equals the hash in `environment.txt`) | 105 runs, 7 reps, pinned, shuffled | I re-derived all medians from `raw.csv`: 0 difference from `summary.csv`/README. 38/105 runs throttled | EXECUTED | Benchmark not rerun on multi-core. VM quota; no `perf c2c`, so the mechanism is inferred. Rerun on bare metal |
| E18 | False sharing 4.31× on AMD EPYC (Task-10) | S21 | Task-10 PDF §9.3 | `phase2_bench.cpp` **absent** | CSV **absent** | Reported only | UNVERIFIED | Commit source and CSV, or drop. Also explain the contrast with Task-7's 0.89× at 2 threads |
| E19 | Atomic < mutex < spinlock at 8 threads (task-8) | S23 | `task-8/` | `locking-benchmark.cpp` | 4 runs, no warm-up, ms resolution | CSV 42/48 rows; analysis averages equal 4-run averages only if the missing values are 14/18/9 and 533/1289/111 ms (unverifiable) | PARTIAL | Restore missing rows; add warm-up, pinning, flags, correct-count check. Not run by me |
| E20 | The SPSC ring buffer's release/acquire protocol is correct | S01 | `Task-9/lockFreeRingBuffer.hpp`; Task-10 PDF §9.4 | same | no correctness test (`sink` never checked) | By inspection both designs publish payload before index with release/acquire | VERIFIED — CODE | Add a test that checks order and sum; run under TSan on multi-core |
| E21 | SPSC beats a mutex queue (Task-9 11.02×; Task-10 6.98×) | — | `Task-9/ringBufferBench.*`; Task-10 PDF | `ringBufferBench.cpp` | 1 run each, 1M ops, ms timing | CSV: 386 ms vs 35 ms. AUD (1 vCPU): ~10–14× | PARTIAL | Direction plausible; magnitude is environment-dependent. Need ≥10 runs, µs timing, recorded hardware. Task-10 numbers unverifiable |
| E22 | The 43.87× result was the compiler deleting the consumer loop | S01 | `Task-9/lockFree.md` | `ringBufferBench.cpp` (pre-fix = commit `8619b79`) | — | AUD: pre-fix 6–7 ms, final 9–10 ms (same order); the screenshot shows the pre-fix 8 ms run | UNVERIFIED | Not supported by my build. Show assembly of the pre-fix loop, or restate the cause as undetermined |
| E23 | Cache misses 12.4M → 3.1M with padding (75%) | S13, S21 | `Task-9/readme.md` | none | no `perf` output | Same value listed for both queue types | UNVERIFIED | Provide `perf stat` output and source, or remove |
| E24 | CAS retry loop is correct and retries grow with threads | S01 | `Task-9/casCounter.cpp`, `cas-results.csv`, screenshot | same | 15 threads: 41,971,194 retries, 2,055 ms; 8 threads: 17,868,468, 917 ms | Counts correct. Screenshot is a different 8-thread run (18,312,541; 922 ms), compiled `-std=c++26` with no `-O`. AUD (1 vCPU): 12 retries | EXECUTED | Hardware unrecorded; no `fetch_add` baseline; CSV `RunID` is 1 on both rows |
| E25 | Tagged pointers prevent ABA; "naive version crashes or livelocks" | S15, S17 | `Task-9/abaDemo.cpp` | tagged stack only | 4×100k push/pop; screenshot | Naive stack never run. Pop reads `old_top.ptr()->next` while another thread may have `delete`d the node (UAF risk by inspection). AUD ASan on 1 vCPU: no report | UNVERIFIED | Needs a deterministic ABA reproducer for the naive stack and a safe-reclamation design (no `delete`, or hazard pointers). Also 16-bit tag wraparound |
| E26 | ABA protection overhead: tagged 22%, hazard 94%, RCU 15% | S15 | `Task-9/abaBenchMarkAnalysis.md` | none | none | No code or data | UNVERIFIED | Remove or implement and measure |
| E27 | Memory reclamation for lock-free structures is handled | S15, S16, S17, S24 | `Task-9/abaReferences.md` | none | none | Sources verified; no implementation | UNVERIFIED | Implement hazard-pointer or epoch demo, or state it as out of scope |
| E28 | Benchmarks follow the Task-11 standard | none cited | `Task-11/methodology/` | `benchmark-template.csv` (header only) | applied to no dataset | Task-7 comes closest; others lack frequency, percentiles, hardware | PARTIAL | Fill the template for each benchmark; add a minimum-sample note (P99 from 7 runs is meaningless); cite a methodology reference |
| E29 | MPMC livelock fixed by `yield()` | none | `Task-9/lockFree.md` | MPMC code **absent** | none | Narrative only. The described state (slot reserved, writer preempted) is a blocking dependency, so that design is not lock-free | UNVERIFIED | Commit the code and a hang reproducer, or remove |
| E30 | Progress guarantees (lock-free / wait-free / obstruction-free) are defined and classified | S15, S18 | none | — | — | Not covered | UNVERIFIED | Add a classification of each structure in the repo |

## Tally
30 claims. VERIFIED — SOURCE 1 · VERIFIED — CODE 2 · EXECUTED 3 · REPRODUCED 0 · PARTIAL 12 · UNVERIFIED 12 · BLOCKED 0.
"Adequately supported" (first three statuses): 6 of 30, and all 6 carry caveats stated above.

## Benchmark records
| Benchmark | Command / build | Environment recorded | Reps | Raw data in repo | Notes |
|---|---|---|---|---|---|
| Task-7 | `run-benchmark.sh results-local 10000000 7 auto`; `g++ -O3 -std=c++20 -pthread` GCC 13.3 | Xeon Platinum 8573C KVM, 9 vCPU, quota 8 CPUs, Ubuntu 24.04.3, kernel 6.18.44 | 7 | yes | Best documented |
| Task5 SB | `g++ -O2 -pthread` (README) | none | 1 | yes (counts) | Controls not run |
| task-8 | none (MinGW GCC; flags unknown) | i5-13500H, Win 11 | 4 | partial (42/48) | |
| Task-9 ring | not recorded; screenshot shows only `./ringBufferBench` | none | 1 | CSV (1 run) | Screenshot is stale |
| Task-9 CAS | screenshot: `g++ -std=c++26` (no `-O`) | none | 1–2 | CSV | |
| Task-10 | `-O2 -std=c++17` (PDF) | AMD EPYC 9V74 KVM 5 CPUs (PDF) | 3 | no | Source/CSV absent |
| Task-4 SB | `-O2` | "x86-64 VM" | 1 | no | Invalid harness |

## Diagram and explanation audit
- **MESI/MOESI state-transition diagram:** none by the team. `Task - 6.pdf` has two flow figures (access flow, read-miss), which I did not visually inspect, and tables of states.
- **Coherence events/messages (BusRd, BusRdX, invalidate):** not found in repo text.
- **Conceptual vs implementation separation:** done well in `Task-4/x86-memory-model.md` (stated as "explanatory model"); not done for fences in `REAADME.md` ("drains the store buffer" is stated flatly).
- **Experimental evidence:** store buffering only (E07). No fence, no coherence, no ARM experiment.

## Highest-priority remaining actions
| # | Action | Depends on | Artifact that closes it |
|---|---|---|---|
| 1 | Rerun Task5 litmus on a ≥4-physical-core machine with both controls | hardware access | `Task5…/results/` with CPU/OS/compiler and control outputs |
| 2 | Retire or fix Task-4 stress test | #1 | Updated file or a note pointing to Task5 |
| 3 | Commit Task-10 sources and CSVs, or remove the claims | Member 10 | `race_demo.cpp`, `spsc_ring_buffer.cpp`, `phase2_bench.cpp`, 2 CSVs |
| 4 | Task-9: raw output, hardware, repetitions, correctness test; remove unsupported tables | Member 9 | Updated CSV with ≥10 runs; test file; deleted/justified tables |
| 5 | ABA: reproducer for naive stack and a safe reclamation | Member 9 | `aba_naive_repro.cpp`, fixed stack, ASan/TSan logs |
| 6 | Restore task-8 CSV rows and record environment | Member 8 | Full 48-row CSV, build line |
| 7 | Add primary ARM source and invalidation-queue note | S08, S18 | Updated Task5 README and a Task-4/6 addendum |
| 8 | Update top-level README (Task-9, Preksha, Richa rows; results) | #3–#6 | Corrected README |
