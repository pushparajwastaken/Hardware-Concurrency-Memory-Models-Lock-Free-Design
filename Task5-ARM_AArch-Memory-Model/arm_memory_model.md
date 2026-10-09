# ARM / AArch64 Memory Model

Short notes on what a memory model is and where ARM sits. For the measured results see [README.md](README.md).

## What is a memory model

A **memory model** is a set of rules for how one part of a computer sees changes made by another part to shared memory.

Think of two people sharing a whiteboard. A writes "The shop is open", then "The door is unlocked". B should read them in that order. If the system lets B see the second line first, B sees an unlocked door and a shop that isn't open yet. The memory model says when that is allowed.

## Why it matters

Modern CPUs have several cores working at once. When two cores share data, we need rules for what each core may see.

```
Core 1                  Core 2

data = 6767             reads ready
ready = true            reads data
```

We expect: if Core 2 sees `ready == true`, it sees `data == 6767`.
Without proper ordering, Core 2 can see `ready == true` and `data == 0`.

```
Core 1 executes:   data = 6767  ->  ready = true
Core 2 observes:   ready = true ->  data = 0     (allowed without ordering)
```

The memory model does not only say what value sits in memory. It says which **orderings other cores are allowed to observe**. The question is not "what did Core 1 run first?" but "what is Core 2 allowed to see?"

## The ARM model

AArch64 uses a **weak (relaxed)** model. The CPU has more freedom to change the order in which memory operations become visible. That helps performance, but the programmer has to ask for ordering when sharing data.

Don't reduce ARM to one label like PSO, RCsc or RCpc. PSO is a named memory model. RCsc and RCpc describe consistency semantics used by some atomic and acquire/release operations. They are related ideas, not names for the ARM model.

### Ordering instructions

| Instruction | Meaning |
|---|---|
| `DMB` | Data Memory Barrier. Orders memory accesses. |
| `DSB` | Data Synchronization Barrier. Waits for relevant operations to complete. |
| `ISB` | Instruction Synchronization Barrier. About instruction fetch and execution, not normal data ordering. |
| `LDAR` | Load-Acquire |
| `STLR` | Store-Release |
| `LDAXR` | Load-Acquire Exclusive |
| `STLXR` | Store-Release Exclusive |

### Acquire / release

```
Core 1: write data -> STLR (release store) -> shared memory -> LDAR (acquire load) -> Core 2: read data
```

- **Release:** earlier memory operations can't be moved after it.
- **Acquire:** later memory operations can't be moved before it.

### Barrier

```
operations before  ->  DMB  ->  operations after
```

Other cores then observe an allowed ordering: everything before the `DMB` is ordered before everything after it.

## Comparison

| | x86-64 | ARM / AArch64 | RISC-V | POWER |
|---|---|---|---|---|
| Model | TSO | Weak / relaxed | RVWMO | POWER model |
| Strength | Relatively strong | Weak | Weak | Very weak |

| | x86-64 | ARM / AArch64 |
|---|---|---|
| Reordering | Less freedom | More freedom |
| Visibility | Usually simpler to reason about | May need explicit ordering |
| Synchronisation | Fewer explicit barriers for common cases | Acquire/release and barriers |
| Programmer effort | Lower | Higher |

The **ideal model** is a learning concept, not a real CPU: every operation becomes visible in exactly the order the program performed it. Real CPUs allow more freedom because it improves performance.

## Sources

1. [Hardware View for Software Hackers](https://www.puppetmastertrading.com/images/hwViewForSwHackers.pdf)
2. [Memory Barriers, CS 5460 Lecture 13](https://users.cs.utah.edu/~aburtsev/cs5460/lectures/lecture13-memory-ordering/lecture13-memory-barriers.pdf)
3. [Memory Reordering Caught in the Act](https://preshing.com/20120515/memory-reordering-caught-in-the-act/)
