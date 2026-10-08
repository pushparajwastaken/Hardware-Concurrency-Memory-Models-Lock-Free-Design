# Performance Benchmark Methodology

## 1. Objective

The objective of this methodology is to establish a reliable,
repeatable, and reproducible approach for measuring the performance
of concurrent and lock-free systems.

The methodology is designed to reduce measurement noise and ensure
that benchmark results include sufficient information about the
hardware, software, and runtime environment.

---

## 2. Scope

The benchmarking process will be used to evaluate performance
characteristics related to:

- Hardware concurrency
- Multi-threaded execution
- Lock-based implementations
- Lock-free implementations
- Atomic operations
- CAS operations
- Cache effects
- False sharing
- Ring-buffer implementations

---

## 3. Environment Metadata

Every benchmark result must record the environment in which it
was executed.

### Hardware

- CPU model
- Number of physical cores
- Number of logical processors
- RAM
- CPU frequency

### Software

- Operating system
- Compiler/runtime version
- Application/benchmark version
- Relevant compiler options

### Runtime Conditions

- CPU utilization
- CPU temperature
- CPU affinity
- NUMA configuration where applicable
- Background workload

---

## 4. Benchmark Design

Each benchmark should define:

- Workload
- Number of threads
- Input size
- Warm-up phase
- Measurement iterations
- Number of repetitions
- Measurement units

The benchmark configuration must remain consistent when comparing
different implementations.

---

## 5. Warm-up

Warm-up iterations are performed before collecting final
measurements.

Warm-up helps reduce the impact of startup effects such as:

- Runtime initialization
- JIT compilation where applicable
- Cache initialization
- Initial system activity

Warm-up results should not normally be included in the final
performance statistics.

---

## 6. Measurements

The benchmark should collect sufficient measurements to identify
normal performance as well as variation.

Important metrics include:

- Execution time
- Latency
- Throughput
- CPU utilization
- CPU frequency
- Temperature

---

## 7. Statistical Analysis

Performance results should not be based on a single measurement.

The analysis should include:

- Minimum
- Maximum
- Mean
- Median / P50
- P90
- P95
- P99

Percentiles are particularly useful for identifying performance
variation and unusually slow executions.

---

## 8. CPU Conditions

CPU frequency and utilization should be recorded because CPU
frequency scaling and system load can influence benchmark results.

Where practical, benchmark execution should be performed under
controlled CPU conditions.

---

## 9. Thermal Conditions

CPU temperature should be monitored during longer or CPU-intensive
benchmarks.

Thermal throttling may reduce CPU frequency and therefore affect
performance measurements.

---

## 10. CPU Affinity

CPU affinity may be controlled when required to reduce variation
caused by thread migration between CPU cores.

The affinity configuration must be documented with the benchmark
results.

---

## 11. NUMA

NUMA configuration should be recorded on systems where NUMA is
present.

NUMA-specific controls should only be applied when relevant to
the hardware and experiment.

---

## 12. Profiling

Where appropriate, profiling tools may be used to investigate
performance bottlenecks.

Potential tools include:

- Linux perf
- Intel VTune
- AMD uProf

Profiling should complement benchmarking rather than replace
controlled benchmark measurements.

---

## 13. Result Validation

A benchmark result should not be considered complete unless the
environment and methodology are documented.

Unexpected results must be investigated before being included in
the final research report.

---

## 14. Suspicious Results

Potentially suspicious results may include:

- Extreme outliers
- Unexpected latency spikes
- Significant variation between repetitions
- Results obtained under abnormal CPU load
- Results affected by thermal throttling
- Results from incorrectly configured benchmarks

Suspicious results should be investigated and documented rather
than silently removed.

---

## 15. Reproducibility

Another researcher or team member should be able to understand
the benchmark configuration and reproduce the experiment using
the documented environment, methodology, source code, and
parameters.

---

## Definition of Done

No benchmark result enters the final research report without:

- Hardware information
- Software information
- Benchmark configuration
- Warm-up information
- Measurement information
- Statistical analysis
- Runtime/environment conditions
- Explanation of any suspicious results
