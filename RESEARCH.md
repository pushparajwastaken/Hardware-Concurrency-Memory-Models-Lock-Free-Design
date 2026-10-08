# What I Read

Two sources, both official documentation. I have not gone through the Intel/AMD manuals or papers for this.

| Source | Sections | What I used it for |
|---|---|---|
| [Linux kernel: False Sharing](https://kernel.org/doc/html/latest/kernel-hacking/false-sharing.html) | What is False Sharing; How to detect and analyze False Sharing; Possible Mitigations | Fields on one line interfere with each other; padding and its cost; `perf c2c` for detection |
| [GCC: Atomic Builtins](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html) | `__ATOMIC_RELAXED`; lock-free support | Relaxed ordering is still atomic, it only drops ordering guarantees between threads |

## How it shows up in the code

- Same line versus separate lines is the whole experiment: `PackedLine` against `PaddedCounter`.
- Aligning only the start of an array does not separate its elements, so `alignas(64)` is on each padded element and `sizeof(PaddedCounter) == 64` is a `static_assert`.
- All modes use the same relaxed `fetch_add`, so the only difference between them is layout.
- The kernel doc recommends `perf c2c`. I could not use it on the VM, so the results are timings only.
