# Hardware Concurrency, Memory Models & Lock-Free Design

A 12-member student research and engineering project on how concurrent C++ code interacts with CPU execution, memory ordering, caches and synchronization primitives. The repository holds research notes, small compiler and assembly experiments, litmus tests, benchmarks and lock-free data-structure programs, organised one folder per team task.

This page is a navigation hub. It states what exists and how much evidence stands behind it; the long-form findings, caveats and reproduction notes are in [`docs/project-findings.md`](docs/project-findings.md). Items without committed output are marked as such rather than assumed to work.

## Contents

1. [Start here](#start-here)
2. [Research questions](#research-questions)
3. [Tasks and where to find them](#tasks-and-where-to-find-them)
4. [Implementations](#implementations)
5. [Experiments and results](#experiments-and-results)
6. [Repository layout](#repository-layout)
7. [Reproducing the work](#reproducing-the-work)
8. [References](#references)
9. [Team](#team)
10. [Limitations](#limitations)
11. [Documentation index](#documentation-index)
12. [Licence and acknowledgments](#licence-and-acknowledgments)

## Start here

| If you want to… | Open |
|---|---|
| See the strongest measured results | [`Task5-ARM_AArch-Memory-Model/README.md`](Task5-ARM_AArch-Memory-Model/README.md) (ARM litmus tests) and [`Task-7/README.md`](Task-7/README.md) (false sharing) |
| Understand the basics first | [`Task1/Hardware_Concurrency_Research1.docx`](Task1/Hardware_Concurrency_Research1.docx) and the [concept map](Task1/Concurrency_Concept_Map.png) |
| Check which claims are actually supported | [`Task-12/evidence-matrix.md`](Task-12/evidence-matrix.md) (note: written before some later work, see [findings §5.4](docs/project-findings.md#54-team-knowledge-transfer)) |
| Read every caveat and limitation | [`docs/project-findings.md`](docs/project-findings.md) |


Headline results, each from committed data with caveats in the linked pages:

- On an AArch64 phone (MediaTek Dimensity 7400 Ultra), relaxed store buffering showed the weak outcome in 11.9–21.4% of 2,000,000 runs per CPU pair, and relaxed message passing in 0.57–2.56%. Release/acquire, `seq_cst` and `dmb` variants showed none. One run per variant and pair, one device ([`summary.txt`](Task5-ARM_AArch-Memory-Model/litmus_results/summary.txt)).
- On a cloud VM, one counter per 64-byte line gave 9.62× the median throughput of packed counters at 8 threads, 3.50× at 4, and no gain at 1–2 threads ([`summary.csv`](Task-7/measured-results/summary.csv)). Hardware counters were not available, so the cause is inferred from the layout change.
- Several other numbers in the repository are single runs, are unrecorded-hardware figures, or are reported in a PDF without source code. [`docs/project-findings.md` §6](docs/project-findings.md#6-experiments--results) lists each one.

## Research questions

The project follows one chain, from application threads down to hardware and back up to performance:

```text
Threads → atomics / synchronization → memory ordering → CPU execution
        → cache hierarchy → cache coherence → shared memory → performance
```

1. How do compiler and CPU reordering differ, and how can each be observed?
2. What do x86-64 (TSO) and ARM/AArch64 (weakly ordered) permit, and can the difference be measured?
3. What does false sharing cost, and does cache-line padding remove it?
4. What do locks, spinlocks and atomics cost under contention?
5. How do CAS loops, ABA and a bounded ring buffer behave, and what does it take to claim a lock-free structure is correct?
6. How should benchmarks be run and reported so that others can trust them?

## Tasks and where to find them

Member attribution is taken from folder numbers, file headers and the roster order; some links are inferred. See [findings §9](docs/project-findings.md#9-team-contributions) for the reasoning.

| Task | Folder | Topic | Contents | Measured data committed? |
|---|---|---|---|---|
| 1 | [`Task1/`](Task1/) | Research foundations | 2 Word documents, concept map, 4 demo programs | No |
| 2 | [`Task-2/`](Task-2/) | CPU execution, compiler vs CPU reordering | Notes, 25-page PDF, experiments 1–4b, `-O0`/`-O2` assembly | Assembly only |
| 3 | [`task_3/`](task_3/) | Store buffers and memory ordering | Theory, case study, paired store-buffering program | No (placeholder results file) |
| 4 | [`Task-4/`](Task-4/) | x86-64 memory model | Notes, recorded assembly, litmus definitions, stress test | No (stress-test output not committed) |
| 5 | [`Task5-ARM_AArch-Memory-Model/`](Task5-ARM_AArch-Memory-Model/) | ARM/AArch64 memory model | Notes, litmus harness, assembly, raw results, 8 plots | Yes |
| 6 | [`Task - 6.pdf`](Task%20-%206.pdf) | MESI/MOESI cache coherence | 7-page literature review | No (review only) |
| 7 | [`Task-7/`](Task-7/) | False sharing | Benchmark, run script, analysis script, raw data, graph | Yes |
| 8 | [`task-8/`](task-8/) | Mutex / spinlock / atomic contention | Benchmark, notes, CSV, graphs | Partial (42 of 48 rows) |
| 9 | [`Task-9/`](Task-9/) | CAS and lock-free algorithms | CAS counter, SPSC ring buffer, ABA demo, race demo | Single-run CSVs |
| 10 | [`Task-10/`](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) | Ring buffer and LMAX Disruptor | PDF report | Reported only, sources not in repo |
| 11 | [`Task-11/`](Task-11/) | Benchmark methodology | Methodology, empty CSV template | No |
| 12 | [`Task-12/`](Task-12/) | Evidence audit, bibliography, KT plan | Evidence matrix, sources, contributions, KT plan | n/a |

## Implementations

"Run status" says what was verified in a re-check with g++ 13.3.0 on a 1-vCPU Linux sandbox. A 1-vCPU machine confirms that programs build and run, but cannot show cross-core behaviour.

| Component | Purpose | Language | Source | Build and run | Run status |
|---|---|---|---|---|---|
| Foundations demos | Memory hierarchy, false sharing, ordering, lock-free basics | C++17 | [`Task1/01…04_*.cpp`](Task1/) | `g++ -O2 -std=c++17 -pthread X.cpp` | Compiled, not run |
| Compiler / ordering experiments | `-O0` vs `-O2` assembly; release/acquire vs relaxed | C++17 | [`Task-2/experiment1–4b.cpp`](Task-2/) | `g++ -O2 -S -masm=intel …`; `g++ -O2 -std=c++17 -pthread experiment4.cpp` | Built and run (print 42) |
| Paired store buffering | Count both-zero outcomes, relaxed vs `seq_cst` | C++20 | [`task_3/practical/store_buffering.cpp`](task_3/practical/store_buffering.cpp) | `g++ -std=c++20 -O2 -pthread … && ./a.out 100000 relaxed` | Built and run (0, as expected on 1 vCPU) |
| x86 stress test | Free-running store-buffering sampler | C++20 | [`Task-4/store-buffering-stress-test.cpp`](Task-4/store-buffering-stress-test.cpp) | `g++ -std=c++20 -O2 -pthread …` | Runs; harness questioned (see Limitations) |
| x86 atomics lowering | C++ orders → x86-64 assembly | C++20 | [`Task-4/x86-atomics.cpp`](Task-4/x86-atomics.cpp) | `g++ -std=c++20 -O2 -S -masm=intel …` | Instruction selection matched the recorded output |
| ARM litmus harness | SB, MP, LB tests, 10 variants | C++17 | [`Task5-ARM_AArch-Memory-Model/arm-litmus.cpp`](Task5-ARM_AArch-Memory-Model/arm-litmus.cpp) | `clang++ -O2 -std=c++17 -pthread` on AArch64 (Termux) | Builds on x86; ARM runs not repeated |
| ARM plots | Weak-outcome table and 8 figures from raw data | Python | [`Task5-ARM_AArch-Memory-Model/plot_litmus.py`](Task5-ARM_AArch-Memory-Model/plot_litmus.py) | `python plot_litmus.py litmus_results --out plots_local` | Reproduced the README percentages |
| False-sharing benchmark | Packed vs padded vs shared counters | C++20, Bash, Python | [`Task-7/`](Task-7/) | `bash run-benchmark.sh <outdir> 10000000 7 auto` | `summary.csv` re-derived from `raw.csv`; smoke test needs ≥2 CPUs |
| Locking benchmark | Mutex vs spinlock vs atomic | C++17 | [`task-8/locking-benchmark.cpp`](task-8/locking-benchmark.cpp) | `g++ -O2 -std=c++17 -pthread …` | Builds |
| CAS counter | Retry loop and retry counts | C++17 | [`Task-9/casCounter.cpp`](Task-9/casCounter.cpp) | `g++ -O2 -std=c++17 -pthread …` (appends to a CSV) | Builds |
| SPSC ring buffer + benchmark | Lock-free queue vs mutex queue | C++17 | [`Task-9/lockFreeRingBuffer.hpp`](Task-9/lockFreeRingBuffer.hpp), [`ringBufferBench.cpp`](Task-9/ringBufferBench.cpp) | `g++ -O2 -std=c++17 -pthread …` (overwrites a CSV) | Built and run |
| Tagged-pointer stack | ABA mitigation (unsafe version never run) | C++17 | [`Task-9/abaDemo.cpp`](Task-9/abaDemo.cpp) | `g++ -O2 -std=c++17 -pthread …` | Builds |
| Race demo | Lost updates on a plain counter | C++17 | [`Task-9/level1_Basics/raceCondition.cpp`](Task-9/level1_Basics/raceCondition.cpp) | `g++ -O0 -std=c++17 -pthread …` | Run: 4,000,000 of 4,000,000 on 1 vCPU (no loss) |

Several of the programs write their output CSVs next to the committed data. Run them in a copy.

## Experiments and results

Correctness-style demonstrations (is an outcome possible?) are kept separate from performance benchmarks (how fast?). Full tables and caveats: [findings §6](docs/project-findings.md#6-experiments--results).

| Experiment | Kind | Research question | Implementation | Results | Main limitation |
|---|---|---|---|---|---|
| ARM litmus tests | Correctness demo | Does AArch64 reorder SB/MP when relaxed, and do `stlr`/`ldar`/`dmb` prevent it? | [`arm-litmus.cpp`](Task5-ARM_AArch-Memory-Model/arm-litmus.cpp) | [`litmus_results/`](Task5-ARM_AArch-Memory-Model/litmus_results/), [`plots/`](Task5-ARM_AArch-Memory-Model/plots/) | One run per cell, one device, pinning success not logged |
| Store buffering, paired | Correctness demo | Same question on a barrier-paired x86 harness | [`store_buffering.cpp`](task_3/practical/store_buffering.cpp) | None committed ([`results.md`](task_3/experiment/results.md) is a template) | No data |
| Store buffering, free-running | Correctness demo | x86 TSO allows store→load reordering | [`Task-4/store-buffering-stress-test.cpp`](Task-4/store-buffering-stress-test.cpp) | Documented figure only, not committed | Harness cannot separate store buffering from its own bookkeeping |
| Lost updates | Correctness demo | Unsynchronised increments lose updates | [`raceCondition.cpp`](Task-9/level1_Basics/raceCondition.cpp) | [Screenshot](Task-9/level1_Basics/Images/raceConditionOutput.png) | Depends on `-O` level and scheduling |
| False sharing | Benchmark | Does 64-byte padding remove cache-line bouncing? | [`Task-7/false-sharing.cpp`](Task-7/false-sharing.cpp) | [`measured-results/`](Task-7/measured-results/) | Throttled cloud VM, no hardware counters |
| Mutex / spinlock / atomic | Benchmark | Which primitive degrades most under contention? | [`locking-benchmark.cpp`](task-8/locking-benchmark.cpp) | [`result.csv`](task-8/result.csv), [`analysis.md`](task-8/analysis.md) | CSV incomplete and disagrees with the averages for 1 and 8 threads |
| CAS retries | Benchmark | How many retries does contention cause? | [`casCounter.cpp`](Task-9/casCounter.cpp) | [`cas-results.csv`](Task-9/cas-results.csv) | Hardware unrecorded, no baseline |
| SPSC ring vs mutex queue | Benchmark | Is the lock-free ring cheaper? | [`ringBufferBench.cpp`](Task-9/ringBufferBench.cpp) | [`ringBufferBench.csv`](Task-9/ringBufferBench.csv) | Single run, millisecond timer, no correctness test |
| Ring buffer, false sharing (Task 10) | Benchmark, reported | Same questions on another VM | Not in the repository | [PDF](Task-10/Ring_Buffer_LMAX_Disruptor_Final_Report.pdf) | No source or CSV, so not reproducible |

## Repository layout

The repository is organised **one folder per team task**, matching the task numbers used in the evidence matrix. There is no top-level build system, test directory or CI configuration.

```text
.
├── README.md
├── docs/
│   ├── project-findings.md       # detailed findings, caveats, run notes (former root README)
│   └── repo-audit.md             # audit, clean-up plan, backlog, validation checklist
├── scripts/check_links.py        # Markdown link checker
├── Task1/                        # research foundations
├── Task-2/                       # CPU reordering, compiler vs CPU
├── task_3/                       # store buffers
├── Task-4/                       # x86-64 memory model
├── Task5-ARM_AArch-Memory-Model/ # ARM/AArch64 memory model (measured)
├── Task - 6.pdf                  # MESI/MOESI literature review
├── Task-7/                       # false-sharing benchmark (measured)
├── task-8/                       # locking benchmark
├── Task-9/                       # CAS, ring buffer, ABA, race demo
├── Task-10/                      # ring buffer / Disruptor report (PDF)
├── Task-11/                      # benchmark methodology
└── Task-12/                      # evidence matrix, sources, contributions, KT plan
```

Folder names are inconsistent (`Task1`, `task_3`, `task-8`, `Task5-…`, and a PDF with spaces in its name). Renaming is proposed in [`docs/repo-audit.md`](docs/repo-audit.md) but has not been applied.

## Reproducing the work

- **Toolchain used for re-checks:** Ubuntu 24.04, g++ 13.3.0, Python 3 with matplotlib 3.10.8. `Task-7` needs Linux or WSL2 and at least two allowed CPUs for multi-thread runs. `Task5` measurements need an AArch64 device with Termux and clang. The committed `.exe` files are Windows binaries and are not needed to rebuild.
- **Smallest meaningful checks (no special hardware):**
  - `cd Task-7 && bash run-benchmark.sh results-local 200000 1 1` runs one thread of the false-sharing benchmark into a fresh folder.
  - `cd Task5-ARM_AArch-Memory-Model && python plot_litmus.py litmus_results --out plots_local` regenerates the ARM table and plots from raw data.
  - `python3 scripts/check_links.py` checks every relative Markdown link.
- **Full instructions and expected outputs:** [findings §10 (How to Run)](docs/project-findings.md#10-how-to-run) and [§12 (Reproducibility)](docs/project-findings.md#12-reproducibility).
- Absolute numbers depend on the CPU, operating system, compiler, thread placement and VM quotas. Do not expect identical results on other hardware.

## References

The bibliography with a verification status per entry is [`Task-12/sources.md`](Task-12/sources.md). Reference lists also sit with the work they support: [`Task1`](Task1/Hardware_Concurrency_References.docx), [`task_3`](task_3/references/references.md), [`Task-7/RESEARCH.md`](Task-7/RESEARCH.md), [`Task-9/abaReferences.md`](Task-9/abaReferences.md), [`task-8/research.md`](task-8/research.md), and the reference sections of the Task 6 and Task 10 PDFs. A consolidated list is in [findings §15](docs/project-findings.md#15-references). The Arm Architecture Reference Manual is listed in `Task1` but not cited in `Task5`.

## Team

Twelve members, one numbered task each. The task-to-member table, the commit-handle mapping and the inferred attributions are in [findings §9](docs/project-findings.md#9-team-contributions); the team's own ledger is [`Task-12/team-contributions.md`](Task-12/team-contributions.md), which predates `Task1`, `task_3` and the ARM update. The proposed knowledge-transfer plan is [`Task-12/kt-plan.md`](Task-12/kt-plan.md); it records no sessions as held.

## Limitations

- **x86 store buffering is not demonstrated.** The only x86 measurement harness ([`Task-4`](Task-4/store-buffering-stress-test.cpp)) is questioned by the team's own audit, and on a 1-vCPU re-check it reported 124,667 of 200,000 both-zero samples although no cross-core store buffering is possible there. The paired program in `task_3` has no committed result.
- **ARM data come from one phone**, one run per variant and CPU pair, with CPU-to-cluster mapping and pinning success unrecorded. Load buffering never appeared, which means "not observed here", not "forbidden".
- **Most benchmarks are single runs on unrecorded or virtualised hardware.** Only `Task-7` follows the methodology in [`Task-11`](Task-11/methodology/benchmark-methodology.md).
- **Lock-free coverage is partial.** No correctness test, ThreadSanitizer run, ABA reproducer, memory-reclamation code or MPMC implementation is committed. Some tables in `Task-9` have no data behind them.
- **No official task specification or professor-feedback document is committed**, so task status here rests on the repository's own records.
- **No standalone MESI/MOESI state-transition diagram is committed.** [`Task - 6.pdf`](Task%20-%206.pdf) contains two flow figures (cache access flow and a read-miss walkthrough).
- `Task-2/notes.md` references four Obsidian images that are not in the repository, and [`task_3/README.md`](task_3/README.md) links a `diagrams/` folder that does not exist.

Details and evidence for each point: [findings §13](docs/project-findings.md#13-limitations--known-gaps).

## Documentation index

| Document | Purpose |
|---|---|
| [`docs/project-findings.md`](docs/project-findings.md) | Per-task summaries, results, key findings, methodology comparison, how to run, lessons, references, status |
| [`docs/repo-audit.md`](docs/repo-audit.md) | Current-state audit, proposed structure and migration table, backlog, validation checklist, implementation order |
| [`Task-12/evidence-matrix.md`](Task-12/evidence-matrix.md) | 30 claims traced to source, code and result |
| [`Task-12/sources.md`](Task-12/sources.md) | Bibliography with verification status |
| [`Task-11/methodology/benchmark-methodology.md`](Task-11/methodology/benchmark-methodology.md) | Benchmarking standard the team set for itself |

## Licence and acknowledgments

The repository contains **no licence file**, so its licensing status is unspecified. The team should decide whether to add one; until then, reuse rights are unclear. The only affiliation stated in the repository is the header "EPAM CoE" on the [`Task1` research note](Task1/Hardware_Concurrency_Research1.docx). No supervisor, institution or funding acknowledgment is recorded; add verified ones here.
