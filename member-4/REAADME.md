# x86-64 Memory Model: TSO, Fences, LOCK, and C++ Atomics → Assembly

Research and hands-on verification of the x86-64 memory model: total-store-order
behavior, `MFENCE`/`LFENCE`/`SFENCE`, `LOCK`-prefixed and implicit-lock atomic
operations, and how C++ `std::memory_order` levels lower to real x86-64 machine code.

## Key findings

- **x86-64 is strongly ordered, not sequentially consistent.** Loads are ordered
  with loads, stores with stores, and a store never passes an older load — but a
  load *may* pass an older store to a **different** address (store→load
  relaxation). This is why the classic **Store Buffering** outcome
  (`r0 == 0 && r1 == 0`) is allowed on real hardware.
- **`MFENCE`, `LFENCE`, `SFENCE` are not interchangeable.** `MFENCE` fences all
  loads+stores (drains the store buffer). `SFENCE` orders stores only — it
  matters mainly around non-temporal (`MOVNT*`) stores. `LFENCE` does **not**
  order memory visibility at all; it serializes instruction execution.
- **Memory-destination `XCHG` is implicitly locked**; `LOCK XADD` / `LOCK CMPXCHG`
  give atomic RMW with full fence semantics and a total order.
- **A C++ memory order is a language contract, not an instruction mnemonic.**
  Measured on GCC 14.2.0 (`-std=c++20 -O2`, default x86-64 target): relaxed and
  acquire loads both emit plain `MOV`; relaxed and release stores both emit
  plain `MOV`; a `seq_cst` store emits `XCHG`; `fetch_add` emits `LOCK XADD`
  for both relaxed and `seq_cst`; a `seq_cst` fence emits
  `lock or QWORD PTR [rsp], 0`. Independently reproduced instruction-identical
  output with GCC 12.2.0.

## Repository structure

| File | Description |
|---|---|
| [`x86-memory-model.md`](x86-memory-model.md) | Full architecture note: TSO ordering principles, fences, locked/RMW ops, C++-to-assembly mapping, MP & IRIW litmus tests, caveats, definition of done |
| [`x86-64-memory-ordering-comparison-table.csv`](x86-64-memory-ordering-comparison-table.csv) | Case-by-case comparison: guarantee, example/mapping, caveat (incl. MP and IRIW rows) |
| [`x86-atomics.cpp`](x86-atomics.cpp) | C++20 source: atomic load/store/`fetch_add`/fence at all memory orders |
| [`x86-atomics-gcc14-x86_64.s`](x86-atomics-gcc14-x86_64.s) | Recorded GCC 14.2.0 output for the source above (assembly snippets) |
| [`x86-memory-litmus-tests.cpp`](x86-memory-litmus-tests.cpp) | Message-passing and IRIW litmus illustrations using C++ atomics |
| [`store-buffering-stress-test.cpp`](store-buffering-stress-test.cpp) | Runnable stress test: empirically observes the store-buffering outcome on real hardware |
| [`REVIEW-AND-CORRECTIONS.md`](REVIEW-AND-CORRECTIONS.md) | Review checklist: what was verified, what was corrected |

## Reproduce

```sh
# Regenerate the assembly snippets
g++ -std=c++20 -O2 -S -masm=intel x86-atomics.cpp
# Syntax-check the litmus illustrations
g++ -std=c++20 -fsyntax-only x86-memory-litmus-tests.cpp
```

- Recorded with **GCC 14.2.0** (Debian 14.2.0-19), default x86-64 Linux target,
  no explicit `-march`.
- Cross-checked during review with **GCC 12.2.0**: identical instruction
  selection for all eight functions.

## Runtime verification

The store→load relaxation is not just theoretical — it is observable. Running
[`store-buffering-stress-test.cpp`](store-buffering-stress-test.cpp) on the
review machine (x86-64 VM, GCC 12.2.0, `-O2`, 2 threads) produced the
sequentially-impossible outcome `r0 == 0 && r1 == 0` in **31,822 of 200,000
sampled rounds (~16%)**. Outcome frequency is machine- and timing-dependent;
a run that sees zero occurrences does not disprove the model.

```sh
g++ -std=c++20 -O2 -pthread store-buffering-stress-test.cpp && ./a.out
```

## Scope and caveats

- All hardware claims apply to **ordinary coherent write-back RAM**. They do
  **not** extend to MMIO, non-temporal operations, or special memory types —
  verify those against the current Intel/AMD SDM.
- The emitted assembly is a **compiler observation**, not an architectural
  guarantee. Always re-verify with your exact compiler, version, flags, target
  CPU, and surrounding code.
- `volatile` is not a substitute for C++ atomics.
- Litmus illustrations define operations; they are not a stress-test harness.

## Sources

Intel® 64 Architecture Memory Ordering White Paper · Owens et al., *A Better
x86 Memory Model: x86-TSO* (2009) · Intel SDM `SFENCE` reference · LLVM Atomic
Instructions and Concurrency Guide · C++ standard `[atomics.order]` · GCC
`__atomic` Builtins documentation. Full citations in
[`x86-memory-model.md`](x86-memory-model.md).
