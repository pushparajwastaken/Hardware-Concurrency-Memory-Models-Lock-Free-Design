# Theoretical Background

## Store Buffers
Store buffers temporarily hold pending writes, allowing CPU cores to continue executing while memory operations are processed.

## Instruction Reordering
Processors and compilers may reorder operations when permitted by the relevant memory model.

## Memory Visibility
One thread may not immediately observe another thread's memory updates without appropriate synchronization.

## Memory Ordering
- Relaxed: Atomicity without additional cross-operation ordering guarantees.
- Acquire: Constrains subsequent operations.
- Release: Constrains preceding operations.
- Sequential consistency: Provides a single total order for sequentially consistent atomic operations.

## Memory Fences
Fences constrain memory-operation ordering according to the language and processor memory models.

## Store Buffering Test
Initially X = 0 and Y = 0.

Thread 1: Store X = 1; Load Y into r1.
Thread 2: Store Y = 1; Load X into r2.

With relaxed atomics, both loads may return 0. With sequentially consistent atomics, this outcome is forbidden.

## Cache Coherence
Cache coherence coordinates cached copies of individual memory locations. It does not replace memory-ordering guarantees.
