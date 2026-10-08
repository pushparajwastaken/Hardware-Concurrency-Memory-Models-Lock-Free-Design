## What actually happens when we run code?
The CPU understands a particular **instruction set architecture (ISA)**.
```             Human-readable
                       │
                       ▼
                 C++ source code
                       │
                       │ compiler
                       ▼
                 Assembly language
                       │
                       │ assembler
                       ▼
                  Machine code
                       │
                       ▼
                     CPU
                       │
                       ▼
              Microarchitecture
```
                 

An instruction is an operation encoded according to the CPU's instruction set that tells the processor what architectural operation to perform.

```
C++ operation
     ↓
compiler transformation
     ↓
machine instructions
     ↓
CPU processes instructions
```

Modern CPUs try to keep many parts of the processor busy simultaneously.

That leads to:
             `CPU`
              `│`
      `┌───────┴────────┐`
      `│                     │`
   `Instruction             Data`
   `processing             movement`
      `│`
      `▼`
   `Pipeline`
      `│`
      `▼`
   `Multiple`
   `instructions`
      `│`
      `▼`
 `Superscalar`
      `│`
      `▼`
 `Out-of-order`
 `execution`

### Why assembly comes before CPU Microarchitecture ?

We need assembly because **the CPU doesn't execute C++ statements**.

It executes instructions.

So if I ask:

> "Can instructions 2 and 3 execute at the same time?"

we need to know what instructions 2 and 3 actually are.
`C++ source`
   `│`
   `│ compiler`
   `▼`
`Assembly / machine instructions`
   `│`
   `│ CPU executes`
   `▼`
`CPU microarchitecture`
   `│`
   `├── pipeline`
   `├── superscalar execution`
   `├── out-of-order execution`
   `├── speculation`
   `└── retirement`
![[Pasted image 20261008194211.png]]

![[Pasted image 20261008194230.png]]

![[Pasted image 20261008194259.png]]

What does mov edx, DWORD PTR 16[rbp] mean?
"Load a value from memory into the `edx` register."
EAX ← EAX + EDX
The important thing is that **EAX is both an input and the destination**.

That creates a dependency

![[Pasted image 20261008195419.png]]
this means:-
```
RCX ──┐
       ├──> addition ──> EAX
RDX ──┘

```
## What actually happens when we code ?
`┌───────────────────────────────┐`
`│ C++ LANGUAGE                             │`
`│ What the programmer writes    │`
`└──────────────┬────────────────┘`
               `│`
               `│ compiler`
               `▼`
`┌───────────────────────────────┐`
`│ ISA INSTRUCTIONS              │`
`│ What instructions exist        │`
`│ MOV, ADD, LEA, JMP, etc.      │`
`└──────────────┬────────────────┘`
               `│`
               `│ CPU implementation`
               `▼`
`┌───────────────────────────────┐`
`│ MICROARCHITECTURE             │`
`│ Pipeline                      │`
`│ Execution units               │`
`│ Scheduler                     │`
`│ OoO execution                 │`
`│ Branch predictor              │`
`│ Reorder buffer                │`
`└───────────────────────────────┘`

# Instruction Level Parallelism 
The program gives the CPU multiple instructions that don't depend on one another.
```

        ┌── I1 ──> execution
CPU ────┤
        └── I2 ──> execution

```

Instead of:

```
I1 → finish → I2 → finish
```

the CPU may be able to do:

```
I1 ─────────>
I2 ─────────>
      time →
```

diff cases:-
```
Case A:

I1 ─────────────>
I2 ─────────────>    potentially parallel


Case B:

I1 ─────────────>
       │
       ▼
I2 ─────────────>    dependency
```


# Pipeline 

Imagine a very simple CPU where every instruction requires four stages:

```
FETCH → DECODE → EXECUTE → WRITEBACK
```

Without pipelining, imagine:

```
Time →

I1: FETCH → DECODE → EXECUTE → WRITEBACK
I2:                               FETCH → DECODE → EXECUTE → WRITEBACK
I3:                                                               ...
```

Only one instruction is progressing through the machine at a time.

With pipelining:

```
Cycle →    1    2    3    4    5    6

I1         F    D    E    W
I2              F    D    E    W
I3                   F    D    E    W
I4                        F    D    E    W
```

Now multiple instructions are **in different stages simultaneously**.

That's the key idea of pipelining.
Pipelining primarily improves **throughput**.

> [!NOTE]
> 
> Latency:
> How long does ONE instruction take?
> 
> Throughput:
> How many instructions can the pipeline process
> per unit of time?
> 

```Pipelining
    ↓
multiple instructions at different stages

Superscalar execution
    ↓
multiple instructions can be issued/executed
in the same cycle 
```


## Pipeline Stall:-
 Now let's make this more realistic.

Suppose:

```
I1: LOAD RAX, [memory]
I2: ADD RAX, RBX
```

Assume, for this simplified model, that `I2` cannot execute until `I1` produces `RAX`.

Without considering any forwarding mechanism, we might get:

```
Cycle →    1    2    3    4    5    6

I1         F    D    E    W
I2              F    D    ?    E    W
```

The `?` represents a **stall**.

Why?

Because `I2` needs the result of `I1`.

This is a **data hazard**.

#### Hazards :-
RAW-Read after write 
WAR-Write after read 
WAW-Write after write 
```
RAW
 ↓
True dependency
 ↓
The later instruction genuinely needs
the value produced by the earlier instruction.
```

```
WAR / WAW
 ↓
Can arise because two instructions
happen to use the same architectural register.
```

RAW is a true data dependency. WAR and WAW are name/dependency hazards that can potentially be eliminated through register renaming.
# Register Renaming

Suppose we have:

```
I1: MOV RAX, RBX
I2: MOV RAX, RCX
```

At the architectural level:

```
Both use RAX.
```

But internally, a modern CPU can potentially map them to **different physical registers**:

```
Architectural RAX
       │
       ├── I1 → Physical R17
       │
       └── I2 → Physical R23
```

Now the CPU doesn't have to treat the two operations as competing for the same physical storage in the same way.

This is a major ingredient of **out-of-order execution**.

# Forwarding

Modern CPUs often don't need to wait for the result to travel all the way through the normal register-write path.

Instead, the result can be **forwarded directly** from where it becomes available to the instruction that needs it.

Conceptually:

```
              result
                │
                ▼
I1 ── Execute ────────┐
                       │
                       │ forwarding
                       ▼
                    I2 Execute
```

Instead of:

```
I1 → Execute → Write Register → Read Register → I2
```

we can conceptually have:

```
I1 → Execute
       │
       └────────→ I2
```

This is called **data forwarding** or **bypassing**.

The exact implementation differs between CPUs, but the fundamental idea is:

> **Don't unnecessarily wait for a result to reach its normal architectural destination if the value is already available somewhere inside the pipeline.**


```
		Dependency
    ↓
The instructions logically depend on each other.

Latency
    ↓
How long it takes for the required result
to become available.
```

### Superscalar Execution 
The CPU can potentially execute/issue **multiple instructions in the same cycle**:

```
Cycle N:

ALU 1 ← I1
ALU 2 ← I2
```

Therefore:

```
Pipeline
    ≠
Superscalar
```

They are related, but they're **different mechanisms**.

And later:

```
Superscalar
      +
Instruction scheduling
      +
Register renaming
      +
Dependency tracking
      ↓
Out-of-order execution
```

That is where the architecture starts getting really interesting.

#### Hard Part 

Suppose the CPU executes:

```
I1 → I3 → I2
```

instead of:

```
I1 → I2 → I3
```

**How does it prevent this from changing the program's visible behavior?**

This is where we need to introduce:

```
Instruction queue
      ↓
Dependency tracking
      ↓
Scheduling
      ↓
Register renaming
      ↓
Reorder Buffer
      ↓
Retirement
```

And **retirement is the key**.

The CPU may execute instructions out of order, but it generally maintains a controlled architectural state and retires instructions in program order, allowing precise exceptions and preserving the ISA-visible behavior.

## Reorder Buffer 

Its fundamental purpose is to let the processor **track instructions in program order while allowing their execution to happen out of order**.


# Register Renaming 
example:

```
I1: MOV RAX, RBX
I2: MOV RAX, RCX
```

We classified it as **WAW**:

```
I1: WRITE RAX
        ↓
I2: WRITE RAX
```

But ask yourself:

> **Do these instructions actually need the same physical storage location inside the CPU?**

Not necessarily.

A modern OoO CPU can use **physical registers** internally.

Conceptually:

```
Architectural register:

RAX
 │
 ├── I1 → Physical Register P7
 │
 └── I2 → Physical Register P12
```

Now:

```
I1 → P7
I2 → P12
```

The two instructions aren't forced to overwrite the same physical register.

This is **register renaming**.
## Why do we need it?

Consider:

```
I1: ADD RAX, RBX
I2: MOV RAX, RCX
```

At the architectural level they appear to conflict:

```
I1 writes RAX
I2 writes RAX
```

But the CPU can internally represent the different versions of `RAX` using different physical registers.

That can eliminate **false dependencies** such as WAR and WAW.

But:

```
I1: ADD RAX, RBX
I2: ADD RCX, RAX
```

has a genuine **RAW dependency**.

Renaming cannot magically remove the fact that I2 needs the value produced by I1.

That's the distinction:

```
FALSE / NAME DEPENDENCY
WAR / WAW
       ↓
register renaming can help


TRUE DATA DEPENDENCY
RAW
       ↓
consumer genuinely needs producer's value
```

# Speculative Execution

Here's the next problem.

Consider:

```
if (x > 10) {
    a = a + 1;
} else {
    b = b + 1;
}
```

The CPU encounters the branch:

```
       x > 10 ?
       /     \
     YES      NO
      │        │
      ▼        ▼
   a = a+1   b = b+1
```

But the CPU may not know immediately which path will be taken.

If it **waits** until the condition is completely resolved before fetching anything after the branch, the pipeline can become inefficient.

So modern CPUs make a prediction:

```
             Branch
                │
         ┌──────┴──────┐
         ▼             ▼
      predicted      other
        path          path
         │
         ▼
    CPU continues
    working here
```

This is **speculative execution**.

And now we have another important distinction:

> **Speculatively executing an instruction does not necessarily mean its result has been committed to architectural state.**

That should sound familiar from our retirement discussion.

Conceptually:

```
Prediction:

Branch
  │
  ├── TRUE ──→ a++  ← speculative
  │
  └── FALSE
        |
Prediction was wrong
        │
        ▼
Discard wrong-path work
        │
        ▼
Restart from correct path
        │
        ▼
      b++
```

This is generally described as a **pipeline flush / recovery from branch misprediction**.

### Why can it discard `a++`?

Because `a++` hasn't necessarily been **retired/committed** yet.

Retirement prevents speculative work from the wrong path from becoming committed architectural state.

## Branch prediction

The CPU makes an educated prediction:

```
Branch
  │
  ▼
Prediction
  │
  ├── TRUE  → fetch/execute TRUE path
  │
  └── FALSE → fetch/execute FALSE path
```

If correct:

```
prediction
    ↓
correct path
    ↓
continue normally
```

If wrong:

```
prediction
    ↓
wrong path
    ↓
misprediction
    ↓
discard wrong-path work
    ↓
recover
    ↓
correct path
```

This is why branch prediction is so important for deep pipelines.

# How does the CPU know what to predict?

It doesn't simply guess randomly.

Modern processors maintain **branch-prediction structures** that learn from previous control-flow behavior.

For example, imagine:

```
for (int i = 0; i < 1000; i++) {
    ...
}
```

The loop's branch behaves roughly like:

```
TRUE
TRUE
TRUE
TRUE
...
TRUE
FALSE
```

A predictor can learn that this branch is **usually taken**.

So when the CPU encounters it again, it predicts:

```
TAKEN
```

This allows the front end to continue fetching instructions without waiting for the branch to resolve.

# Two concepts to distinguish

### Direction prediction

Which way does the branch go?

```
TAKEN
   or
NOT TAKEN
```

### Target prediction

If it is taken:

> **Where should execution continue?**

Conceptually:

```
Branch
  │
  ├── direction → TAKEN
  │
  └── target → address of destination
```

Modern processors use sophisticated mechanisms for both.

# Memory Hierarchy 
```
             FASTEST
                │
          Registers
                │
             L1 Cache
                │
             L2 Cache
                │
             L3 Cache
                │
               RAM
                │
             Storage
                │
             SLOWEST
```

|Level|Relative size|Relative speed|
|---|---|---|
|L1|Small|Very fast|
|L2|Larger|Fast|
|L3|Larger still|Slower|
|RAM|Huge|Much slower|When the CPU requests:

When the CPU requests:
```
array[0]
```

the memory system typically brings a whole **cache line** containing `array[0]` and nearby addresses into the cache.

**Spatial locality** = programs tend to access memory addresses that are close to addresses they recently accessed. 
**Temporal locality** = programs tend to access the **same data again** relatively soon.

#  Multi-core CPUs

So far we've mostly imagined one CPU core:

```
CPU Core
 ├── Registers
 ├── L1
 ├── L2
 └── execution units
```

Modern CPUs have multiple cores:

```
                CPU
     ┌──────────┴──────────┐
     │                     │
   Core 0                Core 1
     │                     │
   L1/L2                 L1/L2
     │                     │
     └──────────┬──────────┘
                │
               L3
                │
               RAM
```

Now imagine two threads:

```
int counter = 0;
```

Thread 1:

```
counter++;
```

Thread 2:

```
counter++;
```

The OS could run them on different cores:

```
Core 0                  Core 1
Thread 1                Thread 2
   │                       │
   │                       │
  L1                       L1
   │                       │
   └─────────┬─────────────┘
             │
            RAM
```

And here's the problem:

**Each core has its own cache.**

So what happens if Core 0 has:

```
counter = 0
```

in its cache, while Core 1 also has:

```
counter = 0
```

in its cache?

If Core 0 changes its cached copy to `1`, Core 1 might still have its own cached copy.

We therefore need a mechanism to keep caches **coherent**.
# Cache coherence

**Cache coherence** is about maintaining a consistent view of a particular memory location across the caches of different cores.

Very simplified:

```
Core 0                 Core 1
  │                       │
  │ counter = 0           │ counter = 0
  │                       │
  │ writes 1              │
  │                       │
  └───────────────►       │
                  "Core 1,
                   your copy
                   is stale"
```

Real CPUs use hardware cache-coherence protocols, such as variants of **MESI**, to coordinate ownership and validity of cache lines.


The important distinction is:

> **Cache coherence answers: "What happens when multiple cores cache the same memory location?"**

But concurrency has another, deeper question:

> **"In what order do memory operations become observable to other threads?"**

That's **memory ordering**, and we'll get there.


|Ordering|`ready` atomic?|Synchronization with `data`?|
|---|---|---|
|Relaxed|Yes|**No**|
|Release + Acquire|Yes|**Yes**, when acquire observes the release|
|Sequentially consistent|Yes|**Yes**, with stronger global ordering|
```
ATOMICITY
"Can this individual atomic operation be observed as
a partially completed operation?"
        ↓
        Atomic

ORDERING
"How are different memory operations ordered?"
        ↓
        Relaxed / Acquire-Release / Seq-Cst

CACHE COHERENCE
"How do different cores maintain coherent copies
of cache lines?"
```

### seq-cst 
All sequentially consistent atomic operations behave as if there is one single global order of those operations, consistent with each thread's own program order.

# Memory fences

Now suppose we want to tell the CPU:

> **"Do not allow certain memory operations to pass this point in the ordering."**

That's where a **memory fence/barrier** comes in.

Conceptually:

```
store A
store B
   │
   ▼
========== FENCE ==========
   │
   ▼
load C
load D
```

A fence imposes ordering constraints on memory operations around it.

This is closely related to what you've already learned:

```
C++ memory ordering
       ↓
compiler + atomic semantics
       ↓
ISA memory-ordering instructions
       ↓
CPU hardware
       ↓
store buffers / caches / coherence
```

Different architectures provide different mechanisms and have different memory-ordering models.

For example, **x86-64 is relatively strongly ordered compared with architectures such as ARM**, which is one reason a concurrency bug may appear on one architecture and not another.

But:

> **"It works on my x86 machine" is not a proof that the C++ program is correct.**

The C++ memory model determines what your program is allowed to assume.

|Order|Main idea|Use when|
|---|---|---|
|`relaxed`|Atomicity, minimal ordering|Counters/statistics where synchronization isn't needed|
|`release`|Publishes previous writes|Producer/thread sending data|
|`acquire`|Consumes a release|Consumer/thread receiving data|
|`acq_rel`|Acquire + release|Read-modify-write operations needing both|
|`seq_cst`|Strongest/simple global ordering|When simplicity/correctness matters more than weaker ordering|

# Compiler transformations, CPU out-of-order execution, and memory ordering are different layers. A compiler may transform instructions, a CPU may execute independent instructions out of order, while the memory model defines the ordering guarantees that concurrent threads can rely upon.