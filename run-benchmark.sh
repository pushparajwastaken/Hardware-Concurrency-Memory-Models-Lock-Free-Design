#!/usr/bin/env bash
# Linux / WSL2; GCC 11+ with C++20 atomic wait support.
set -euo pipefail
cd "$(dirname "$0")"
out="${1:-results-local}"
iterations="${2:-10000000}"
repeats="${3:-7}"
counts="${4:-auto}"
mkdir -p "$out"
if [[ -e "$out/raw.csv" ]]; then
  echo "Output already exists: $out/raw.csv. Choose a new output directory." >&2
  exit 1
fi
compiler="${CXX:-g++}"
line_bytes="${CACHE_LINE_BYTES:-64}"
flags=(-O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic "-DCACHE_LINE_BYTES=$line_bytes")
"$compiler" "${flags[@]}" false-sharing.cpp -o "$out/false-sharing"
{
  echo "UTC start: $(date -u +%FT%TZ)"
  echo "Build: $compiler ${flags[*]} false-sharing.cpp -o $out/false-sharing"
  echo "Run: $out/false-sharing $iterations $repeats $counts"
  uname -a
  cat /etc/os-release
  "$compiler" --version
  ldd --version
  lscpu
  echo "Allowed CPUs:"
  taskset -pc $$
  echo "Cache line sizes:"
  for f in /sys/devices/system/cpu/cpu0/cache/index*/coherency_line_size; do
    echo "$f: $(cat "$f")"
  done
  for name in cpu.max cpu.stat cpuset.cpus.effective; do
    echo "cgroup $name:"
    cat "/sys/fs/cgroup/$name" 2>/dev/null || true
  done
  echo "perf executable: $(command -v perf || echo unavailable)"
  echo "Sources/binary SHA-256:"
  sha256sum false-sharing.cpp "$out/false-sharing"
} > "$out/environment.txt"
"$out/false-sharing" "$iterations" "$repeats" "$counts" > "$out/raw.csv" 2> "$out/benchmark.log"
{
  echo "UTC finish: $(date -u +%FT%TZ)"
  echo "cgroup cpu.stat after benchmark:"
  cat /sys/fs/cgroup/cpu.stat 2>/dev/null || true
} >> "$out/environment.txt"
echo "Saved $out/raw.csv, $out/environment.txt and $out/benchmark.log"
