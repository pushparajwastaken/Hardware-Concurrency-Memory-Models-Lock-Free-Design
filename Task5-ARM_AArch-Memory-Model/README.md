# Observing Memory Reordering on x86-64 and ARM (AArch64)

CPUs reorder memory operations. This repo catches it happening, first on an x86-64 PC and then on an AArch64 Android phone. It also shows which C++ memory orders and fences stop it.

It starts from Jeff Preshing's *Memory Reordering Caught in the Act*. That test got data logging and plotting. On top of it come ARM litmus tests (SB, MP, LB) and a file of tiny atomic functions for reading the assembly.

---

## Contents

| File | Purpose |
|---|---|
| `ordering_pinned.cpp` | Original x86 store-buffering test, extended with 1M-run logging, a warm-up phase and thread pinning |
| `plot_reorders.py` | Plots the x86 data: cumulative reorders vs runs and reorders per second (single data set) |
| `plot_litmus.py` | Plots the ARM litmus results for several CPU pairs on shared graphs, plus summary comparison graphs |
| `arm-litmus.cpp` | ARM litmus tests: SB, MP, LB, each with several memory orders and fences |
| `arm-atomics.cpp` | One tiny function per atomic operation, for reading the assembly |
| `arm-atomics.s` | AArch64 assembly (ARMv8.0, GCC 13) |
| `arm-atomics-lse.s` | AArch64 assembly with `-march=armv8.1-a` (LSE atomics) |
| `arm_memory_model.md` | Short notes on what a memory model is and how ARM compares with x86 |
| `litmus_results/` | Raw ARM litmus data: `summary.txt` plus one folder per CPU pair (`cpu0-1`, `cpu6-7`, `cpu0-7`) |
| `plots/` | Generated figures (see [section 6](#6-results-by-cpu-pair-pinned-runs)) |

---

## Test environment

All ARM results come from this phone:

| | |
|---|---|
| Hardware model | RMX5110 |
| SoC | MediaTek Dimensity 8400 Ultra 5G (4 nm) |
| CPU | 8x Cortex-A725 (ARMv9.2-A): 1 at 3.25 GHz, 3 at 3.0 GHz, 4 at 2.1 GHz |
| GPU | Mali-G720 MC7 |
| RAM | 6 GB |
| OS / shell | Android, Termux |
| Compiler | `clang++ -O2 -std=c++17 -pthread` |

ARMv9.2-A includes everything in ARMv8, so the ARMv8 memory model discussed here applies.
All eight cores are the same Cortex-A725 design. They differ only in clock speed and cache
setup, so "slow pair" and "fast pair" below means clock tiers, not little vs big cores.
Chip details from [91mobiles](https://www.91mobiles.com/processor/mediatek-dimensity-8400-ultra-pdp).

To see which CPU numbers sit in which clock tier on your own device:

```
for c in /sys/devices/system/cpu/cpu[0-7]; do echo "$(basename $c) $(cat $c/cpufreq/cpuinfo_max_freq)"; done
```

The x86 test in section 2 ran on a Windows PC (MSYS2 UCRT64). Its CPU details are not recorded here yet.

---

## 1. Background: memory models

A CPU or compiler can reorder memory operations as long as a single thread can't tell the difference. Another thread can. That is why concurrent code needs ordering rules.

| | x86-64 | ARMv8 (AArch64) |
|---|---|---|
| Model | Total Store Order (TSO), "strong" | Relaxed, "weak" |
| Load → Load reordering | No | **Yes** |
| Load → Store reordering | No | **Yes** |
| Store → Store reordering | No | **Yes** |
| Store → Load reordering | **Yes** | **Yes** |
| Store atomicity | Multicopy atomic | Multicopy atomic since the ARMv8 revision (older ARM was not) |

x86 only reorders **Store → Load**. The cause is the store buffer, and the original program detects exactly that. ARM allows all four combinations. Code that works on x86 by accident can break on ARM.

ARM has a formal concurrency model. The architecture was revised to be multicopy atomic, and a formal model was added to the spec (Pulte et al., POPL 2018, see References).

### Ordering tools on AArch64

| C++ construct | AArch64 instruction | What it guarantees |
|---|---|---|
| `relaxed` load / store | `ldr` / `str` | Atomic, but no ordering |
| `acquire` load | `ldar` | Later accesses cannot move before it |
| `release` store | `stlr` | Earlier accesses cannot move after it |
| `seq_cst` load / store | `ldar` / `stlr` | Same instructions as acquire / release on ARMv8 |
| `atomic_thread_fence(seq_cst)` | `dmb ish` | Full barrier |
| `atomic_thread_fence(acquire)` | `dmb ishld` | Orders earlier loads before later loads and stores |
| `atomic_thread_fence(release)` | `dmb ish` (GCC) | Orders earlier accesses before later stores |
| `fetch_add(seq_cst)` | `ldaddal` (LSE) or `ldxr`/`stxr` loop | Atomic read-modify-write |
| `exchange(seq_cst)` | `swpal` (LSE) | Atomic swap |
| `compare_exchange(seq_cst)` | `casal` (LSE) | Compare and swap |

Notes from the generated assembly:

- ARMv8 hardware orders `stlr` before a later `ldar`, though the C++ standard doesn't promise that for release/acquire. That is why "SB release/acquire" shows zero weak outcomes below.
- GCC 10.1+ uses outline atomics on AArch64 by default, so plain `fetch_add` calls a helper like `__aarch64_ldadd4_acq_rel`. Compile with `-mno-outline-atomics` to see the inline `ldxr`/`stxr` loop.
- `asm volatile("" ::: "memory")` emits no instruction. It only stops the **compiler** from reordering.

---

## 2. Part 1: x86-64 store-buffering test

Two threads each run a store followed by a load on the other thread's variable:

```
Thread 1:  X = 1;  r1 = Y;
Thread 2:  Y = 1;  r2 = X;
```

If `r1 == 0 && r2 == 0`, both loads finished before either store became visible. That is a Store → Load reorder. Sequential consistency makes this outcome impossible.

Changes to the original program:

- Stop after 1,000,000 runs, logging data in memory and writing it at the end.
- `reorders_vs_runs.txt`: cumulative reorders, sampled every 1,000 runs.
- `reorders_per_second.txt`: reorders and runs in each second of runtime.
- A **warm-up phase** (200,000 uncounted runs) so cold caches, frequency ramp-up and thread migration don't skew the first second.
- **Thread pinning** (`SetThreadAffinityMask` on Windows) with configurable cores.
- `USE_CPU_FENCE 1` (`mfence`) removes the reorders, as expected.

Build on Windows (MSYS2 UCRT64) with `g++`, not `gcc`. The code uses `std::vector`:

```
g++ -O2 -o ordering.exe ordering_pinned.cpp -lpthread
python plot_reorders.py
```

### Initial run (before warm-up and pinning)

| Metric | Value |
|---|---|
| Total runs | 1,000,000 |
| Total reorders | about 1,520 (about 0.15% of runs) |
| Mean reorders per second | 126.5 over 12 seconds |
| First second | about 306 reorders, then it settled around 60 to 190 |

The first-second spike is why the warm-up phase and pinning exist. Graph 1 plots the cumulative curve against fitted linear and log reference curves, plus a log-log view.

---

## 3. Part 2: ARM litmus tests

`arm-litmus.cpp` runs three classic tests. Each has two threads, a spin barrier to start them together, and a random start offset (0 to 127 nop steps) to try different alignments. Each run records `(r0, r1)`. The **weak outcome** is the one sequentially consistent execution can never produce.

| Test | Thread 0 | Thread 1 | Weak outcome |
|---|---|---|---|
| **SB** (store buffering) | `x = 1; r0 = y` | `y = 1; r1 = x` | `r0 == 0 && r1 == 0` |
| **MP** (message passing) | `data = 1; flag = 1` | `r0 = flag; r1 = data` | `r0 == 1 && r1 == 0` |
| **LB** (load buffering) | `r0 = x; y = 1` | `r1 = y; x = 1` | `r0 == 1 && r1 == 1` |

Variants tested:

| Test | Variants |
|---|---|
| SB | relaxed, release/acquire, seq_cst, relaxed with `dmb ish` |
| MP | relaxed, release/acquire, relaxed with fences, writer-only release |
| LB | relaxed, store-release |

### Build and run (Termux on Android)

```
pkg install clang
cp arm-litmus.cpp ~/ && cd ~
clang++ -O2 -std=c++17 -pthread arm-litmus.cpp -o litmus

./litmus 2000000                 # one unpinned run of every test
./litmus 2000000 0:1 2:5 6:7     # one run per CPU pair (thread 0 on the first CPU, thread 1 on the second)
./litmus 2000000 auto            # pick three pairs automatically (see below)
./litmus 2000000 auto run2       # same, writing into ./run2 instead of ./results
```

`auto` reads each CPU's max frequency from `/sys/devices/system/cpu/cpuN/cpufreq/` and picks three pairs: the two slowest CPUs, the two fastest, and one slow plus one fast. On a big.LITTLE phone that compares little-to-little, big-to-big and cross-cluster. If the frequencies can't be read, or all CPUs are equal, it falls back to `0:1`, `(n-2):(n-1)` and `0:(n-1)`. The chosen pairs are printed at the start. Each test runs once per pair, so three pairs take about three times as long.

Build and run from Termux's home directory. Shared storage like `Download` is mounted non-executable and gives `Permission denied`.

### Result files and plotting

After each run, `arm-litmus.cpp` writes one folder per CPU pair and one sub-folder per test:

```
results/
  summary.txt                 one line per (pair, test): iterations, weak count, weak %, runtime, all four outcome counts
  cpu0-1/
    SB_relaxed/
      reorders_vs_runs.txt        "runs cumulative_reorders", sampled every 1,000 runs
      reorders_per_second.txt     "second reorders runs_in_that_second"
    SB_release_acquire/ ...
    MP_relaxed/ ...
  cpu6-7/ ...
  cpu0-7/ ...
```

Plot everything with `plot_litmus.py`. Each CPU pair gets its own colour on every graph:

```
pip install numpy matplotlib
python plot_litmus.py results            # writes PNGs into ./plots
python plot_litmus.py results --show     # also opens the windows
```

| Figure | What it shows |
|---|---|
| `SB_cumulative.png`, `MP_cumulative.png`, `LB_cumulative.png` | Cumulative weak outcomes vs runs. One panel per instruction variant, one line per CPU pair, shared y-axis so variants compare directly |
| `SB_per_second.png`, `MP_per_second.png`, `LB_per_second.png` | Weak outcomes in each second of runtime, same layout. The last (partial) second is dropped |
| `rate_by_instruction.png` | Weak-outcome percentage for all ten variants, grouped by test, one bar per CPU pair |
| `outcome_mix.png` | Share of runs in each `(r0,r1)` outcome per variant, one row per CPU pair, weak outcome hatched in red |

The script also prints a table of weak-outcome percentages (variants against CPU pairs). Variants
with no weak outcomes show a flat zero line labelled "no weak outcomes observed".

The per-second graphs show how the rate changes during a test, for example a warm-up spike. Pairs run at different speeds, so compare overall rates with the percentage graphs, not raw counts. To keep separate runs, give an output folder (`./litmus 2000000 auto run2`) and plot it with
`python plot_litmus.py run2 --out plots_run2`.

To read the assembly:

```
g++ -O2 -S arm-atomics.cpp -o arm-atomics.s
g++ -O2 -S -march=armv8.1-a arm-atomics.cpp -o arm-atomics-lse.s
```

---

## 4. Terminal output

Device: the RMX5110 phone from [Test environment](#test-environment), 8 hardware threads, no pinning, 2,000,000 iterations per test.

```
Architecture: AArch64, hardware threads: 8, iterations per test: 2000000
Pinning: none (pass two CPU numbers to pin)
Outcome columns are (r0,r1). The weak outcome is marked with '*'.

SB relaxed               [str; ldr]
   (0,0)*=414980    (0,1) =1583956   (1,0) =1063      (1,1) =1
   weak outcome: 414980 of 2000000 (20.7490%)  -> OBSERVED

SB release/acquire       [stlr; ldar]
   (0,0)*=0         (0,1) =1701452   (1,0) =9616      (1,1) =288932
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

SB seq_cst               [stlr; ldar]
   (0,0)*=0         (0,1) =1700189   (1,0) =9225      (1,1) =290586
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

SB relaxed + dmb ish     [str; dmb ish; ldr]
   (0,0)*=0         (0,1) =1691302   (1,0) =11401     (1,1) =297297
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

MP relaxed               [str; str  /  ldr; ldr]
   (0,0) =314887    (0,1) =42553     (1,0)*=69897     (1,1) =1572663
   weak outcome: 69897 of 2000000 (3.4949%)  -> OBSERVED

MP release/acquire       [str; stlr  /  ldar; ldr]
   (0,0) =311270    (0,1) =71631     (1,0)*=0         (1,1) =1617099
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

MP relaxed + fences      [str; dmb; str  /  ldr; dmb ishld; ldr]
   (0,0) =406951    (0,1) =1219239   (1,0)*=0         (1,1) =373810
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

MP only writer release   [str; stlr  /  ldr; ldr (reader weak)]
   (0,0) =343869    (0,1) =45807     (1,0)*=35202     (1,1) =1575122
   weak outcome: 35202 of 2000000 (1.7601%)  -> OBSERVED

LB relaxed               [ldr; str]
   (0,0) =424077    (0,1) =1575184   (1,0) =739       (1,1)*=0
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed

LB store-release         [ldr; stlr]
   (0,0) =430820    (0,1) =1568230   (1,0) =950       (1,1)*=0
   weak outcome: 0 of 2000000 (0.0000%)  -> not observed
```

---

## 5. Results (unpinned run)

Counts are out of 2,000,000 iterations per test. The weak outcome is marked with `*`.

### Summary

| Test | Variant | Instructions | Weak outcome count | Weak rate | Verdict |
|---|---|---|---:|---:|---|
| SB | relaxed | `str; ldr` | 414,980 | 20.7490% | **Observed** |
| SB | release/acquire | `stlr; ldar` | 0 | 0% | Not observed |
| SB | seq_cst | `stlr; ldar` | 0 | 0% | Not observed |
| SB | relaxed + `dmb ish` | `str; dmb ish; ldr` | 0 | 0% | Not observed |
| MP | relaxed | `str; str` / `ldr; ldr` | 69,897 | 3.4949% | **Observed** |
| MP | release/acquire | `str; stlr` / `ldar; ldr` | 0 | 0% | Not observed |
| MP | relaxed + fences | `str; dmb; str` / `ldr; dmb ishld; ldr` | 0 | 0% | Not observed |
| MP | writer release only | `str; stlr` / `ldr; ldr` | 35,202 | 1.7601% | **Observed** |
| LB | relaxed | `ldr; str` | 0 | 0% | Not observed |
| LB | store-release | `ldr; stlr` | 0 | 0% | Not observed |

### Full outcome counts

| Test | Variant | (0,0) | (0,1) | (1,0) | (1,1) |
|---|---|---:|---:|---:|---:|
| SB | relaxed | **414,980\*** | 1,583,956 | 1,063 | 1 |
| SB | release/acquire | 0\* | 1,701,452 | 9,616 | 288,932 |
| SB | seq_cst | 0\* | 1,700,189 | 9,225 | 290,586 |
| SB | relaxed + `dmb ish` | 0\* | 1,691,302 | 11,401 | 297,297 |
| MP | relaxed | 314,887 | 42,553 | **69,897\*** | 1,572,663 |
| MP | release/acquire | 311,270 | 71,631 | 0\* | 1,617,099 |
| MP | relaxed + fences | 406,951 | 1,219,239 | 0\* | 373,810 |
| MP | writer release only | 343,869 | 45,807 | **35,202\*** | 1,575,122 |
| LB | relaxed | 424,077 | 1,575,184 | 739 | 0\* |
| LB | store-release | 430,820 | 1,568,230 | 950 | 0\* |

---

## 6. Results by CPU pair (pinned runs)

Same tests, but the two threads are pinned to specific CPUs. 2,000,000 iterations per test, per pair. 🟥 means the weak (reordered) outcome was seen. 🟩 means never seen. Raw data is in [`litmus_results/`](litmus_results/). Figures are in [`plots/`](plots/), made by `plot_litmus.py`.

### At a glance: weak-outcome rate

| Test | Variant | Instructions | CPUs 0 & 1 | CPUs 6 & 7 | CPUs 0 & 7 |
|---|---|---|---:|---:|---:|
| SB | relaxed | `str; ldr` | 🟥 **11.9253%**<br><sub>238,506</sub> | 🟥 **21.3815%**<br><sub>427,629</sub> | 🟥 **17.8945%**<br><sub>357,890</sub> |
| SB | release/acquire | `stlr; ldar` | 🟩 0 | 🟩 0 | 🟩 0 |
| SB | seq_cst | `stlr; ldar` | 🟩 0 | 🟩 0 | 🟩 0 |
| SB | relaxed + `dmb ish` | `str; dmb ish; ldr` | 🟩 0 | 🟩 0 | 🟩 0 |
| MP | relaxed | `str; str` / `ldr; ldr` | 🟥 **0.5653%**<br><sub>11,306</sub> | 🟥 **2.5596%**<br><sub>51,192</sub> | 🟥 **0.6633%**<br><sub>13,266</sub> |
| MP | release/acquire | `str; stlr` / `ldar; ldr` | 🟩 0 | 🟩 0 | 🟩 0 |
| MP | relaxed + fences | `str; dmb; str` / `ldr; dmb ishld; ldr` | 🟩 0 | 🟩 0 | 🟩 0 |
| MP | writer release only | `str; stlr` / `ldr; ldr` | 🟥 **0.0040%**<br><sub>79</sub> | 🟥 **2.1524%**<br><sub>43,049</sub> | 🟥 **0.0006%**<br><sub>11</sub> |
| LB | relaxed | `ldr; str` | 🟩 0 | 🟩 0 | 🟩 0 |
| LB | store-release | `ldr; stlr` | 🟩 0 | 🟩 0 | 🟩 0 |

> **Short version:** only the unordered variants break. Anything with `stlr`/`ldar` on both sides, or a `dmb` fence, stayed at zero on all three pairs. *Writer release only* is the trap. It still leaks on CPUs 6 & 7 because the reader is plain `ldr; ldr`.

### Weak-outcome rate by instruction and CPU pair

![Weak-outcome rate for every variant and CPU pair](plots/rate_by_instruction.png)

SB relaxed sits between 12% and 21%. MP relaxed sits between 0.6% and 2.6%. Everything with proper ordering is flat at zero. The CPU pair changes how big the effect is, not which variants are affected. The one oddity is MP writer-release-only: 79 and 11 hits on two pairs, but clearly visible on 6 & 7.

### Where every run ended up

![Outcome mix per variant, one row per CPU pair](plots/outcome_mix.png)

Each bar is 100% of the runs for one variant, split into the four `(r0,r1)` outcomes. The red hatched part is the weak outcome. The rest of the mix differs a lot between pairs. SB on CPUs 6 & 7 is mostly `(0,1)`. On CPUs 0 & 7 it is split between `(0,1)` and `(1,0)`. That comes from which thread the harness starts first, not from the memory model. So use the rate graphs to compare pairs, not this one.

### Store buffering (SB)

![SB cumulative weak outcomes](plots/SB_cumulative.png)

The relaxed curves are straight lines. The weak outcome happens at a steady rate for the whole run, not in bursts. Release/acquire, `seq_cst` and `dmb ish` are all empty.

<details>
<summary>SB weak outcomes per second</summary>

![SB per second](plots/SB_per_second.png)

</details>

### Message passing (MP)

![MP cumulative weak outcomes](plots/MP_cumulative.png)

<details>
<summary>MP weak outcomes per second</summary>

![MP per second](plots/MP_per_second.png)

Each test finishes in about one second (0.55 to 1.3 s), so these graphs have only one or two points per line. They become useful on longer runs, e.g. `./litmus 20000000 auto`.

</details>

### Load buffering (LB)

![LB cumulative weak outcomes](plots/LB_cumulative.png)

<details>
<summary>LB weak outcomes per second</summary>

![LB per second](plots/LB_per_second.png)

</details>

Both LB variants are empty on all pairs. Same as the unpinned run.

### Full outcome counts per pair

<details>
<summary>Show all 30 rows (the weak outcome is marked with *)</summary>

| Pair | Test | Variant | (0,0) | (0,1) | (1,0) | (1,1) | Runtime |
|---|---|---|---:|---:|---:|---:|---:|
| CPUs 0 & 1 | SB | relaxed | **238,506\*** | 920,408 | 840,609 | 477 | 1.032 s |
| CPUs 0 & 1 | SB | release/acquire | 0\* | 734,278 | 842,149 | 423,573 | 1.103 s |
| CPUs 0 & 1 | SB | seq_cst | 0\* | 715,930 | 860,605 | 423,465 | 1.082 s |
| CPUs 0 & 1 | SB | relaxed + `dmb ish` | 0\* | 769,922 | 800,107 | 429,971 | 1.072 s |
| CPUs 0 & 1 | MP | relaxed | 1,092,130 | 69,155 | **11,306\*** | 827,409 | 1.046 s |
| CPUs 0 & 1 | MP | release/acquire | 848,294 | 471,459 | 0\* | 680,247 | 1.281 s |
| CPUs 0 & 1 | MP | relaxed + fences | 839,866 | 486,495 | 0\* | 673,639 | 1.174 s |
| CPUs 0 & 1 | MP | writer release only | 1,147,513 | 256,694 | **79\*** | 595,714 | 1.073 s |
| CPUs 0 & 1 | LB | relaxed | 464,696 | 719,453 | 815,851 | 0\* | 1.041 s |
| CPUs 0 & 1 | LB | store-release | 804,914 | 519,086 | 676,000 | 0\* | 1.133 s |
| CPUs 6 & 7 | SB | relaxed | **427,629\*** | 1,557,647 | 14,723 | 1 | 0.561 s |
| CPUs 6 & 7 | SB | release/acquire | 0\* | 1,620,740 | 49,914 | 329,346 | 0.634 s |
| CPUs 6 & 7 | SB | seq_cst | 0\* | 1,586,139 | 69,617 | 344,244 | 0.634 s |
| CPUs 6 & 7 | SB | relaxed + `dmb ish` | 0\* | 1,633,959 | 29,679 | 336,362 | 0.625 s |
| CPUs 6 & 7 | MP | relaxed | 349,815 | 92,104 | **51,192\*** | 1,506,889 | 0.547 s |
| CPUs 6 & 7 | MP | release/acquire | 397,827 | 97,757 | 0\* | 1,504,416 | 0.554 s |
| CPUs 6 & 7 | MP | relaxed + fences | 1,144,890 | 603,028 | 0\* | 252,082 | 0.590 s |
| CPUs 6 & 7 | MP | writer release only | 402,995 | 99,647 | **43,049\*** | 1,454,309 | 0.576 s |
| CPUs 6 & 7 | LB | relaxed | 464,379 | 1,526,206 | 9,415 | 0\* | 0.626 s |
| CPUs 6 & 7 | LB | store-release | 445,978 | 1,551,055 | 2,967 | 0\* | 0.650 s |
| CPUs 0 & 7 | SB | relaxed | **357,890\*** | 555,898 | 1,085,849 | 363 | 0.795 s |
| CPUs 0 & 7 | SB | release/acquire | 0\* | 544,045 | 1,112,629 | 343,326 | 0.866 s |
| CPUs 0 & 7 | SB | seq_cst | 0\* | 572,027 | 1,076,569 | 351,404 | 0.837 s |
| CPUs 0 & 7 | SB | relaxed + `dmb ish` | 0\* | 578,666 | 1,079,324 | 342,010 | 0.841 s |
| CPUs 0 & 7 | MP | relaxed | 1,302,744 | 57,712 | **13,266\*** | 626,278 | 0.786 s |
| CPUs 0 & 7 | MP | release/acquire | 1,315,285 | 257,339 | 0\* | 427,376 | 0.844 s |
| CPUs 0 & 7 | MP | relaxed + fences | 1,245,476 | 299,738 | 0\* | 454,786 | 0.843 s |
| CPUs 0 & 7 | MP | writer release only | 1,280,292 | 281,009 | **11\*** | 438,688 | 0.842 s |
| CPUs 0 & 7 | LB | relaxed | 501,751 | 443,171 | 1,055,078 | 0\* | 0.832 s |
| CPUs 0 & 7 | LB | store-release | 644,085 | 271,448 | 1,084,467 | 0\* | 0.863 s |

</details>

---

## 7. Analysis

- **SB relaxed (20.7%).** Plain `str` then `ldr`. The load finishes before the store is visible. Same Store → Load reordering as x86.
- **SB with `stlr`/`ldar` is clean.** ARMv8 hardware orders a release store before a later acquire load. That is stricter than C++ requires. `dmb ish` forbids the outcome outright.
- **MP relaxed (3.5%)** can't happen on x86. Either the writer's two stores reach other cores out of order, or the reader's two loads run out of order, or both.
- **MP release/acquire and MP with fences are clean.** Order both sides and it's fixed.
- **MP writer-release-only (1.76%)** is the practical lesson. A correct writer doesn't help if the reader uses plain loads. The reader's `ldr; ldr` can still reorder.
- **CPU pair changes the rate, not the verdict.** SB relaxed goes from 11.9% (CPUs 0 & 1) to 21.4% (CPUs 6 & 7). Every ordered variant is zero on all three pairs.
- **Writer-release-only depends on the pair.** 43,049 hits (2.15%) on CPUs 6 & 7, but only 79 and 11 on CPUs 0 & 1 and 0 & 7. Test only one of those pairs and this broken code looks safe. That is why several pairs matter.
- **LB at zero.** The architecture allows load buffering. This chip doesn't show it, or it is too rare to catch. Zero here doesn't mean forbidden.

### Reading the other outcome columns

In SB relaxed, almost all non-weak runs land in `(0,1)` (about 1.58M) and almost none in `(1,0)` (1,063). So thread 0 usually ran before thread 1. The spin barrier is not symmetric. The weak outcomes come from runs where the threads really overlapped. The fenced SB variants show about 290k runs in `(1,1)`. That is where overlapping runs end up once the weak outcome is forbidden. So the rates belong to this harness and this device. They are not universal.

---

## 8. Caveats

- **Only three CPU pairs.** Section 6 pins to CPUs 0 & 1, 6 & 7 and 0 & 7. Section 5 is the older unpinned run, so don't compare the two directly. Android may refuse affinity changes. Then the program prints a warning and runs unpinned.
- **Empirical only.** "Not observed" is what this chip did in these runs. It is not what the architecture forbids. For that, use the formal model tool [rmem](https://www.cl.cam.ac.uk/~sf502/regressions/rmem/). It needs the tests written as AArch64 assembly litmus files.
- **Asymmetric start.** The harness skews which thread starts first, as described above.
- **Run-to-run variation.** Temperature, background load and frequency scaling change the results. Repeat runs before comparing small differences.

---

## References

- Pulte, Flur, Deacon, French, Sarkar, Sewell. *Simplifying ARM Concurrency: Multicopy-Atomic Axiomatic and Operational Models for ARMv8.* POPL 2018. https://www.cl.cam.ac.uk/~pes20/armv8-mca
- Preshing. *Weak vs. Strong Memory Models.* https://preshing.com/20120930/weak-vs-strong-memory-models
- Preshing. *Memory Reordering Caught in the Act* (source of the original test). https://preshing.com/20120515/memory-reordering-caught-in-the-act/
- Arm Learning Paths. *Learn about the C++ memory model for porting applications to Arm.* https://learn.arm.com/learning-paths/servers-and-cloud-computing/arm-cpp-memory-model/1/
- Zhang et al. *Weak Memory Models: Balancing Definitional Simplicity and Implementation Flexibility.* https://arxiv.org/pdf/1707.05923
- Intel. *Intel 64 Architecture Memory Ordering White Paper* (x86 reordering rules, document 318147). https://www.cs.cmu.edu/~410-f10/doc/Intel_Reordering_318147.pdf
- rmem: interactive tool for the formal ARM, POWER and RISC-V concurrency models. https://www.cl.cam.ac.uk/~sf502/regressions/rmem/
