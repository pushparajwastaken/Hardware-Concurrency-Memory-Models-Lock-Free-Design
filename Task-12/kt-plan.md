# kt-plan.md — Knowledge-transfer plan (Member 12)

**Status: PROPOSED. No KT session has been held or recorded in the repo.** The only KT artifacts found are a 12-question checklist for Member 10 (Task-10 PDF §14) and a review file for Member 4. Dates and presenters below are suggestions pending confirmation.

## A. Objectives
1. Share one understanding of the C++ memory model: data races, happens-before, the five orders.
2. Separate language guarantees, compiler transformations and hardware behaviour.
3. Know what coherence guarantees and what it does not.
4. Explain store buffers, invalidation handling and fences without overclaiming (architectural guarantee vs implementation detail).
5. Reproduce each PoC and state its limits (see `evidence-matrix.md`).
6. Read a benchmark and spot invalid conclusions.
7. Review and extend each other's work.

## B. Session agenda
Sessions are 60 minutes unless noted. "Demo" lists only things that exist today; missing items are marked.

| # | Topic | Prerequisites | Presenter / owner (proposed) | Duration | Demonstration | Expected outcome | Preparation material |
|---|---|---|---|---|---|---|---|
| 1 | Repo orientation, requirements, evidence statuses | none | Member 12 + repo owner | 45 min | Walk the README, `evidence-matrix.md` | Everyone can find any claim's evidence | README, this folder |
| 2 | Atomics, data races, happens-before | 1 | Task-2 owner | 60 | `Task-9/level1_Basics/raceCondition.cpp` (run) | Explain why lost updates occur and why a pass proves nothing | S01, S02, `Task-2/notes.md` |
| 3 | Relaxed vs acquire/release vs seq_cst | 2 | Task-4 owner | 60 | `Task-2/experiment4/4b` — show **why they cannot fail** and that 4b is UB | Predict allowed outcomes of MP and SB per order | S01, `x86-memory-litmus-tests.cpp` |
| 4 | Compiler reordering vs hardware ordering | 3 | Task-2 owner | 60 | `Task-2` O0/O2 asm; `Task-4` atomics asm | Tell apart folding, cmov, and CPU reordering | S11, `Task-4/x86-atomics*.{cpp,s}` |
| 5 | Store buffers, invalidation mechanisms, fences | 4 | Task-4 + Task-5 owners | 75 | Task5 litmus run (needs ≥3 cores) and the `mfence` control (**not yet run**: do this first) | Explain SB; say what MFENCE guarantees vs what Intel's store-buffer text says; know invalidation queues are **missing** from the repo | S03, S05, S14, S18 |
| 6 | MESI/MOESI and coherence ≠ ordering | 5 | Task-6 owner | 60 | Hand-drawn transition table (**no team diagram exists**; produce one) | Draw M/E/S/I transitions for a read miss and a write to a Shared line | `Task - 6.pdf`, S18 |
| 7 | PoC execution and result interpretation | 5 | Task-5, Task-4, Task-7 owners | 90 | Run Task5 and Task-7 smoke test; show Task-4 all-`seq_cst` control | Reproduce results; state why Task-4's 16% was retired | `evidence-matrix.md` E06–E08, E17 |
| 8 | Benchmark methodology and reproducibility | 7 | Task-11 owner | 60 | Apply the Task-11 template to Task-7, then to task-8 and Task-9 | Complete a template row; list confounders (VM quota, no pinning, ms timing) | `Task-11/methodology`, Task-7 `environment.txt` |
| 9 | Lock-free correctness, CAS, ABA, reclamation, progress guarantees | 3, 8 | Task-9 and Task-10 owners | 90 | `casCounter`, SPSC ring (run). **Missing**: ABA reproducer, reclamation, MPMC | Argue the SPSC release/acquire protocol; explain why `delete` after pop is unsafe; classify progress guarantees | S15, S16, S17, S24 |
| 10 | Evidence review, gaps, final integration | all | Member 12 + owners | 60 | Walk the 30 matrix rows; assign closure of each UNVERIFIED row | Signed-off action list | `evidence-matrix.md` |

Total: about 10 hours. Compress 2–4 into one session if time is short; do not drop 5, 7 or 9.

## C. Execution checklists
**Presenter preparation:** read the owning artifacts and the cited sources; run every demo on the session machine beforehand; list known limits of each result; prepare two questions that could expose a weak claim.
**Environment setup:** Linux or WSL2; GCC 13+; ≥4 logical CPUs for Tasks 5/7; Python 3 with matplotlib for plots; `perf` if available; record `lscpu`, kernel, compiler, flags.
**Code walkthrough:** state the hypothesis; show where the compiler barrier or atomic order sits; point out every non-atomic shared variable; note what is measured and what is inferred.
**Live demonstration:** run unmodified first; then the control; save output to a file; do not edit results.
**Participant questions:** each participant asks one "what would falsify this?" question.
**Independent reproduction:** a different member rebuilds from a clean clone, using only the README; record CPU/OS/compiler, command, output, differences.
**Recording findings:** one note per session in `docs/member-12/kt-log/session-N.md` (date, attendees, what was run, outcome of the check, follow-ups). Do not log a session that did not happen.
**After the session:** update matrix statuses (REPRODUCED only with a second person's recorded run); fix doc errors found; open follow-up items.

## D. Knowledge verification
| Session | Check | Pass criterion |
|---|---|---|
| 2 | Why can `counter++` from 4 threads print 4,000,000 once? | Names the race and UB; says tests cannot prove absence |
| 3 | Given MP with relaxed flag, list allowed outcomes in C++ and on x86 hardware | Distinguishes abstract model from x86 |
| 4 | Classify three snippets: constant fold, cmov, CPU reordering | All correct with a reason |
| 5 | Explain why Task5 compiler barrier did not stop (0,0); what `mfence` changes | Uses ordering semantics; does not claim "flushes cache" |
| 6 | Draw MESI read-miss and write-to-Shared transitions | Correct states and bus events |
| 7 | Reproduce Task-7 medians from `raw.csv`; explain why Task-4's control broke the test | Matches within rounding; identifies the main-thread reset |
| 8 | Review `task-8` against the template | Finds ≥5 gaps (no warm-up, pinning, flags, missing rows, ms timing) |
| 9 | Find the use-after-free window in `abaDemo.cpp`; propose a fix | Points to `old_top.ptr()->next` after `delete` |
| 10 | Pick an UNVERIFIED row; state the artifact that would close it | Concrete and testable |

## E. Priority gaps this plan targets
Fence control not run (S5, S7); invalidation queues absent (S5, S6); no team coherence diagram (S6); Task-4 harness (S7); ABA/reclamation (S9); benchmark hygiene (S8). Closing actions and dependencies are tabulated at the end of `evidence-matrix.md`.

## Sessions held
None.
