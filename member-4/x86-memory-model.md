# x86-64 Memory Model: Research Brief (FINAL)

**Review status:** Both C++ sources compile cleanly (`g++ -std=c++20 -fsyntax-only`, GCC 12.2.0). The compiler mapping table was independently reproduced: GCC 12.2.0 emits instruction-for-instruction identical code to the supplied GCC 14.2.0 output for all eight functions, corroborating that these mappings are stable across these two GCC versions for this source and flags. All factual claims reviewed against the cited sources; see `REVIEW-AND-CORRECTIONS.md` for the full checklist.

## Purpose and scope

This brief addresses the assigned x86-64 memory-model work: x86 ordering/TSO, `MFENCE`/`LFENCE`/`SFENCE`, `LOCK`-prefixed operations, atomic read-modify-write, and the mapping from C++ orders to machine code. The requested work products are the architecture note, assembly examples, and C++-to-assembly comparison.

## Core result

For ordinary coherent write-back memory, x86-64 is strongly ordered but not sequentially consistent in every execution: loads are ordered with loads; stores with stores; a store is not reordered with an older load; but a load may pass an older store to a *different* location. This store→load relaxation explains the classic Store Buffering outcome. Intel's published ordering principles also describe total order for writes to the same location and locked instructions, and prohibit ordinary loads/stores from being reordered with locked instructions. [^1]

```text
Initially x = y = 0
Thread 0:                 Thread 1:
  x = 1                    y = 1
  r0 = y                   r1 = x

On x86-64, r0 = 0 and r1 = 0 is allowed: each load can pass its own earlier store to a different address.
```

A useful operational intuition is per-core store buffers: a core can forward its own buffered store to its own load, while another core cannot yet observe that store. This is an explanatory model, not a claim that implementations must use a particular internal structure. Formal x86-TSO work models the architecture using local write buffers and an equivalent axiomatic account. [^1][^2]

## Fences and locked operations

The three fence instructions are **not** interchangeable names for a generic compiler barrier; they order different things. Exact semantics and special cases must be verified in the current Intel/AMD SDM before relying on them (see caveats below). [^3][^4]

- **`MFENCE` — full memory fence.** Serializes all load and store ordering: every load and store before the `MFENCE` is globally visible/complete before any load or store after it. This includes draining the store buffer — it is the fence that closes the store→load relaxation for surrounding accesses. LLVM documents an `MFENCE` lowering for sequentially consistent fences on x86. [^3]
- **`SFENCE` — store fence only.** Prior stores become globally visible before later stores. On ordinary coherent WB memory, stores are already strongly ordered, so `SFENCE` matters mainly around **non-temporal (streaming) stores** (`MOVNT*`), which bypass the store buffer and are *not* otherwise ordered; it does not order loads at all. [^4]
- **`LFENCE` — not a load-ordering memory fence.** `LFENCE` does not order load or store visibility between processors; it serializes *instruction execution*: no instruction after the `LFENCE` begins execution until all instructions before it have completed locally. (It was also repurposed in published speculation-control mitigations.) Do not use `LFENCE` as a substitute for `MFENCE` where load/store ordering is required — that is exactly the kind of special case the SDM check exists for.

Other atomicity primitives:

- A memory-destination `XCHG` is **implicitly locked** — no `LOCK` prefix is written. `LOCK`-prefixed read-modify-write operations (e.g. `LOCK XADD`, `LOCK CMPXCHG`) have the ordering/atomicity properties described for locked instructions: they are not reordered with ordinary loads/stores and participate in a total order. Use language atomics for C++ synchronization; do not add `LOCK` by hand to ordinary C++ accesses. [^1]

The available Intel ordering reference confirms atomicity/order properties of locked instructions and identifies its scope as coherent write-back memory; it explicitly excludes I/O ordering and certain instruction classes. Consequently, the WB-memory summary above must not be applied to MMIO, non-temporal operations, or special memory types without checking the current Intel SDM. [^1]

## C++ memory order is not an instruction mnemonic

C++ specifies a language-level contract; the compiler selects instructions for a particular target, compiler version, optimization level, and surrounding code. `relaxed` keeps the individual atomic access indivisible but creates no inter-thread ordering; release/acquire can establish synchronization when the acquire reads from the release sequence; `seq_cst` participates in the standard's single total order for sequentially consistent operations. The standard's strongest sequential-consistency guarantee has conditions: it does not make racy ordinary accesses valid, and mixing weaker orders needs careful reasoning. [^5]

GCC likewise describes target-specific mappings, and permits fallback calls when an operation cannot be emitted as a lock-free instruction sequence. Therefore "C++ order X always equals instruction Y" is not a valid general rule. [^6]

### Measured compiler example

These examples were compiled locally with **GCC 14.2.0, `g++ -std=c++20 -O2 -S -masm=intel`, default x86-64 target**. They describe that exact test configuration, not every compiler or microarchitecture. The complete source is supplied separately as `x86-atomics.cpp`; generated output is supplied as `x86-atomics-gcc14-x86_64.s`.

| C++ operation | Relevant generated instruction (this build) | Interpretation |
|---|---|---|
| relaxed atomic load | `mov eax, DWORD PTR x[rip]` | Ordinary load suffices for this native-width atomic load in this output. |
| acquire atomic load | `mov eax, DWORD PTR x[rip]` | Same machine instruction as relaxed here; the acquire contract still constrains compiler/hardware-visible behavior. |
| relaxed / release atomic store | `mov DWORD PTR x[rip], edi` | Same instruction for both in this output. |
| sequentially consistent store | `xchg edi, DWORD PTR x[rip]` | The compiler selected an implicitly locked memory exchange. |
| relaxed / sequentially consistent `fetch_add` | `lock xadd DWORD PTR x[rip], eax` | Atomic RMW is locked in both cases; the stronger order need not produce a distinct instruction here. |
| sequentially consistent thread fence | `lock or QWORD PTR [rsp], 0` | GCC emitted a locked no-op read-modify-write as its full-fence implementation in this build; do not assume it will always use this sequence. |

**Independent reproduction (this review):** GCC 12.2.0 with the identical command produces instruction-for-instruction identical output for all eight functions. This corroborates the table for these two GCC versions on this source; it is still not an architectural guarantee.

LLVM's target-specific documentation gives a different illustrative lowering strategy for x86 — e.g., describing an `MFENCE` for sequentially consistent fences — and explicitly identifies x86 atomic load/store/RMW mappings. This underscores why the compiler and exact target need to be named when documenting emitted code. [^3]

## Deliverable boundaries and caveats

The assembly is a reproducible compiler observation, not an architectural guarantee. Re-run with the actual production compiler, flags, target triple/CPU, and surrounding source before using it as implementation evidence. `volatile` is not a substitute for C++ atomics; inline assembly must express its compiler memory effects as well as any hardware instruction semantics. This note focuses on ordinary coherent write-back RAM and standard C++ atomics, not device I/O, persistence, transactional memory, or non-temporal-store protocols. The assigned topics and deliverables are taken from the supplied task screenshot. [^7]

## Definition of done

- [x] **Architecture explanation:** state x86-64/TSO allowed ordering and the key store→load relaxation, with the Store Buffering example.
- [x] **Fence/atomic coverage:** explain the distinct role of `MFENCE`, `LFENCE`, `SFENCE`, implicit memory `XCHG`, and locked RMW operations; require SDM verification for special memory types.
- [x] **C++ comparison:** distinguish the C++ abstract memory-order contract from compiler lowering; compare relaxed/acquire/release/seq_cst examples.
- [x] **Reproducible snippets:** provide compilable C++ and generated assembly with compiler, version, flags, and target context recorded.
- [x] **Claim calibration:** qualify compiler-dependent mappings and identify WB memory and excluded cases.
- [ ] **Project-specific sign-off:** rerun the snippets with the project's exact compiler/version/flags/CPU target and check against the applicable current Intel/AMD manuals; fill this in before treating the assembly as the project's expected output.

The deliverable is complete as a research baseline. It is not project-validated until the final unchecked sign-off is completed for the actual build and hardware specification.

## Additional litmus tests: message passing and IRIW

The attached `x86-memory-litmus-tests.cpp` uses atomic objects throughout, so its relaxed-ordering versions are meaningful C++ executions rather than examples that accidentally invoke undefined behavior through racy ordinary variables. These routines illustrate the operations and outcomes; they are not a stress-test harness and cannot guarantee an outcome will appear in a finite run.

### Message passing (MP)

Initially `payload = 0` and `ready = 0`.

```text
Writer:                    Reader:
payload = 1                r_ready = ready
ready = 1                  if (r_ready == 1) r_payload = payload

Questioned outcome: r_ready == 1 && r_payload == 0
```

For x86-64 ordinary write-back memory, this outcome is forbidden: the writer's two stores remain ordered, and the reader's two loads remain ordered; if it sees the publication store, it must also see the preceding payload store. A formal x86 example states this implication directly for the corresponding two-location store-then-load test. [^8]

With C++ relaxed atomics, the outcome is permitted because the flag operations do not establish inter-thread ordering. Replacing the flag store with `memory_order_release` and its load with `memory_order_acquire` makes the observed `ready == 1` synchronize the threads; the preceding payload store then happens-before the payload load, so it must read `1` (assuming no later payload stores). [^5][^9]

### IRIW (Independent Reads of Independent Writes)

Initially `x = y = 0`; all variables below are atomic. The writers each update one location. The two readers read the locations in opposite orders:

```text
Writer 0: x = 1           Writer 1: y = 1
Reader 0: r0x = x; r0y = y
Reader 1: r1y = y; r1x = x

Split-view outcome: r0x == 1 && r0y == 0 && r1y == 1 && r1x == 0
```

That outcome would mean the readers disagree about which independent store became visible first. It is forbidden by x86's consistent global store order; a C++ relaxed-atomic IRIW execution may allow it because relaxed operations do not establish a single total order across locations. The standard's SC operations, by contrast, participate in a single total order; using `seq_cst` for all four operations in each test forbids the split-view outcome. [^10][^5]

| Test | Outcome of interest | x86-64 hardware (coherent WB RAM) | C++ relaxed atomics | C++ all-`seq_cst` variant |
|---|---|---|---|---|
| Message passing | Sees flag=1 but stale payload=0 | Forbidden by store-store and load-load ordering. [^8] | Permitted: relaxed flag accesses do not synchronize. [^5] | Forbidden; SC atomic operations carry stronger synchronization/order guarantees. [^5] |
| IRIW | Readers observe opposite orders for x=1 and y=1 | Forbidden; x86 provides a consistent store order across observers. [^10] | Permitted in the C++ abstract model. [^10] | Forbidden by the single total order over SC operations. [^5] |

**Interpretation caution:** hardware litmus outcomes and C++ abstract-machine permissions answer different questions. A stronger target may never exhibit an outcome the C++ model permits, while compiler transformations and language rules still matter. For a portable correctness argument, use the C++ synchronization contract; for a machine-code claim, inspect the compiled program and use the target ISA model.

## Files in this deliverable

1. `x86-memory-model.md` — this note.
2. `x86-64-memory-ordering-comparison-table.csv` — condensed comparison table.
3. `x86-atomics.cpp` — compilable demonstration source.
4. `x86-atomics-gcc14-x86_64.s` — recorded compiler output for that source.
5. `x86-memory-litmus-tests.cpp` — MP and IRIW litmus illustrations.
6. `REVIEW-AND-CORRECTIONS.md` — review checklist and the corrections applied to reach this final version.

This repository layout matches the deliverable names in the assigned task sheet: `x86-memory-model.md` (architecture note), assembly snippets (`.s` + `.cpp` sources), and the comparison table (`.csv`).
7. `README.md` — manifest and reproduction instructions.

[^1]: sjkuo. Intel® 64 Architecture Memory Ordering White Paper.
[^2]: Owens et al., 2009. A Better x86 Memory Model: x86-TSO (Extended Version).
[^3]: LLVM Atomic Instructions and Concurrency Guide.
[^4]: SFENCE — Store Fence.
[^5]: [atomics.order].
[^6]: __atomic Builtins (Using the GNU Compiler Collection (GCC)).
[^7]: Screenshot 2026-10-08 195317.png.
[^8]: The Semantics of x86 Multiprocessor Machine Code.
[^9]: Thread coordination using Boost.Atomic.
[^10]: Will two atomic writes to different locations in different threads always be seen in the same order by other threads?.
