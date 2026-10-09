# Review and Corrections Report — x86-64 Memory Model Deliverable

Date: 2026-10-08. Scope: the five working files supplied for review, consolidated into the final set.

## Checks performed

| # | Check | Result |
| --- | --- | --- |
| 1 | `x86-atomics.cpp` compiles (`g++ -std=c++20 -fsyntax-only`, GCC 12.2.0) | PASS |
| 2 | `x86-memory-litmus-tests.cpp` compiles (same) | PASS |
| 3 | Assembly independently regenerated (`g++ -std=c++20 -O2 -S -masm=intel`) and compared to the recorded GCC 14.2.0 output | PASS — instruction-for-instruction identical for all 8 functions |
| 4 | Store Buffering, MP, IRIW outcomes vs. x86-TSO literature claims in the brief | PASS — match Intel ordering white paper / x86-TSO results |
| 5 | C++ memory-order semantics ([atomics.order]) summarized correctly | PASS |
| 6 | C++ `memory_order` ↔ instruction mapping presented as compiler observation, not architecture guarantee | PASS |
| 7 | Fences: MFENCE / LFENCE / SFENCE individually explained | **FAIL in draft → FIXED (see corrections)** |
| 8 | WB-memory scope and excluded cases (MMIO, NT stores) stated | PASS (strengthened) |
| 9 | Duplicate/variant files resolved into one final set | FIXED |
| 10 | Definition-of-done sign-off status honest | PASS — project-specific sign-off remains open, flagged |

## Corrections applied to reach the final version

1. **LFENCE gap (content bug).** The draft named `LFENCE` as "distinct" but never explained what it does. The final brief now states explicitly that `LFENCE` does not order load/store *visibility* (it is not a load-ordering memory fence): it serializes instruction execution, and must not be substituted for `MFENCE` where memory ordering is required.
2. **SFENCE precision.** Added that on ordinary WB memory `SFENCE` matters mainly around non-temporal (streaming) stores (`MOVNT*`), which bypass the store buffer, and that it does not order loads.
3. **Redundant variants removed.** Two copies of the brief and two copies of the comparison table existed (with/without the litmus section and MP/IRIW rows). The final set keeps exactly one brief (complete, with litmus section) and one comparison table (complete, with MP/IRIW rows). No content was lost.
4. **Independent reproduction recorded.** GCC 12.2.0 was used to regenerate the assembly; output is instruction-identical to the recorded GCC 14.2.0 listing. This is now stated in the brief as corroboration — while keeping the caveat that it remains a compiler observation, not an architectural guarantee.
5. **Minor polish.** Removed a stray placeholder glyph after "comparison." in the draft's Purpose section; normalized the final file list; kept the unchecked project-sign-off item explicitly open rather than silently completing it.

## Items intentionally left open (not defects)

- **Project-specific sign-off:** the snippets must be rerun with the project's exact compiler/version/flags/CPU and checked against the current Intel/AMD SDM. This is a task-owner action, flagged in the brief's definition of done.
- The recorded assembly is GCC-specific; LLVM's documented `MFENCE`-based lowering for SC fences differs and is cited as such.

## Post-review addition: runtime evidence

| # | Check | Result |
|---|---|---|
| 11 | Store Buffering executed as a live stress test (2 threads, relaxed atomics, 200k rounds) | PASS — store→load relaxation observed empirically: 31,822/200,000 rounds (~16%) returned `r0 == 0 && r1 == 0` on the review machine (GCC 12.2.0, -O2, x86-64 VM) |

New file added: `store-buffering-stress-test.cpp` (runnable, self-terminating). This closes the gap
between the static litmus illustrations and observable hardware behavior. Still open, by design:
project-specific compiler/SDM sign-off (item in the brief's definition of done).
