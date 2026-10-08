# False Sharing and Cache-Line Bouncing

My part of **From Theory to Silicon Proof** (item 2): compare counters that sit next to each other with counters that each get their own cache line.

## What it tests

Every thread increments its own counter, so logically nothing is shared. The only thing I change is where the counters live in memory.

| Mode | Layout | Why it is there |
|---|---|---|
| `packed` | 8-byte counters back to back, eight per 64-byte line | False sharing |
| `padded` | One counter per 64-byte line (`alignas(64)`) | The fix |
| `true_shared` | All threads increment one counter | Control: real sharing |

All three modes run the same loop, `fetch_add(1, std::memory_order_relaxed)`, 10 million times per thread. Threads are pinned to CPUs and start together from a gate. Each case is repeated 7 times in shuffled order, and the final counter values are checked after every run.

## Results

Median throughput in million increments per second:

| Threads | Packed | Padded | True shared | Padded / packed |
|---:|---:|---:|---:|---:|
| 1 | 136.2 | 131.4 | 132.7 | 0.96x |
| 2 | 123.2 | 110.3 | 123.5 | 0.89x |
| 4 | 64.8 | 227.0 | 65.6 | 3.50x |
| 8 | 43.4 | 417.1 | 43.8 | 9.62x |
| 9 | 49.6 | 439.4 | 42.2 | 8.86x |

![Throughput and padding benefit](measured-results/false-sharing-graph.png)

- At 8 threads padding is about 9.6x faster, and packed is as slow as one truly shared counter.
- At 1 and 2 threads padding does not help. At 2 threads it was actually a bit slower.
- With 9 threads the packed counters spill onto a second line, so that row is not a clean comparison with 8.

**Where these numbers come from:** a cloud Linux VM (Xeon Platinum 8573C under KVM, 9 vCPUs, GCC 13.3), not my laptop. The VM had a CPU quota and some runs were throttled, so the exact ratios are noisy. A rerun on my own machine is still to be added. `perf` was not available, so cache-line transfers are inferred from the layout comparison and not counted directly.

More detail is in [false-sharing-results.md](false-sharing-results.md). Raw data is in [measured-results/](measured-results/).

## Run it

Needs Linux or WSL2 with GCC 11+ (C++20). It uses Linux affinity calls, so it will not build with MSVC as it is.

```bash
bash run-benchmark.sh results-local 10000000 7 auto

# graph and summary (needs matplotlib)
pip install -r requirements.txt
python3 analyze.py results-local
```

Arguments are output folder, increments per thread, repeats, and thread counts (`auto` or a list like `1,2,4,8`). The script will not overwrite an existing `raw.csv`. If your CPU's cache line is not 64 bytes, run with `CACHE_LINE_BYTES=<size>`.

## Files

- [false-sharing.cpp](false-sharing.cpp): the benchmark
- [run-benchmark.sh](run-benchmark.sh): builds, records the environment, runs
- [analyze.py](analyze.py): checks the CSV, writes `summary.csv` and the graph
- [measured-results/](measured-results/): raw CSV, summary, graph, environment, log and the hot-loop disassembly
- [RESEARCH.md](RESEARCH.md): what I read

I used AI assistance while building this.
