// Linux + C++20. See false-sharing-results.md for methodology and limitations.
// Build: g++ -O3 -std=c++20 -pthread -Wall -Wextra -Wpedantic false-sharing.cpp -o false-sharing
// Run: ./false-sharing 10000000 7 > raw.csv 2> benchmark.log
// Arguments: iterations/thread, repeats, optional comma-separated thread counts,
//            optional mode (all, packed, padded, true_shared).
// Override cache-line assumption when compiling: -DCACHE_LINE_BYTES=128
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <pthread.h>
#include <sched.h>

#ifndef CACHE_LINE_BYTES
#define CACHE_LINE_BYTES 64
#endif
constexpr std::size_t line_bytes = CACHE_LINE_BYTES;
using Counter = std::atomic<std::uint64_t>;
using Clock = std::chrono::steady_clock;
static_assert(Counter::is_always_lock_free, "Requires lock-free 64-bit atomics");
static_assert(sizeof(Counter) == 8, "This benchmark expects 8-byte counters");
static_assert(line_bytes >= sizeof(Counter) && (line_bytes & (line_bytes - 1)) == 0);
constexpr std::size_t per_line = line_bytes / sizeof(Counter);

// Alignment of the first element alone does NOT separate the elements.
struct alignas(line_bytes) PackedLine { Counter values[per_line]{}; };
// Aligning each element makes the array stride one full line.
struct alignas(line_bytes) PaddedCounter { Counter value{0}; };
static_assert(sizeof(PackedLine) == line_bytes);
static_assert(sizeof(PaddedCounter) == line_bytes);
struct alignas(line_bytes) ThreadResult { Clock::time_point end; int pin_error = 0; };
struct alignas(line_bytes) Flag { std::atomic<int> value{0}; };

std::vector<int> allowed_cpus() {
    cpu_set_t mask; CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) != 0)
        throw std::runtime_error("sched_getaffinity failed");
    std::vector<int> cpus;
    for (int i = 0; i < CPU_SETSIZE; ++i) if (CPU_ISSET(i, &mask)) cpus.push_back(i);
    if (cpus.empty()) throw std::runtime_error("No CPUs available");
    return cpus;
}
std::uint64_t number(const std::string& s) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected a positive integer");
    auto n = std::stoull(s);
    if (!n) throw std::runtime_error("Arguments must be positive");
    return n;
}
std::string cpu_string(const std::vector<int>& cpus, int n) {
    std::ostringstream out;
    for (int i = 0; i < n; ++i) { if (i) out << ';'; out << cpus[i]; }
    return out.str();
}
long long throttled_us() {
    std::ifstream in("/sys/fs/cgroup/cpu.stat");
    std::string key; long long value;
    while (in >> key >> value) if (key == "throttled_usec") return value;
    return -1;
}
struct Measurement { double seconds; std::uint64_t sum; std::size_t lines; };

Measurement run(const std::string& mode, int n, std::uint64_t iterations,
                const std::vector<int>& cpus, bool describe) {
    auto packed = std::make_unique<PackedLine[]>((n + per_line - 1) / per_line);
    auto padded = std::make_unique<PaddedCounter[]>(n);
    PaddedCounter shared;
    std::vector<Counter*> targets(n);
    for (int i = 0; i < n; ++i) {
        if (mode == "packed") targets[i] = &packed[i / per_line].values[i % per_line];
        else if (mode == "padded") targets[i] = &padded[i].value;
        else targets[i] = &shared.value;
    }
    std::set<std::uintptr_t> lines;
    auto base = reinterpret_cast<std::uintptr_t>(targets[0]);
    for (int i = 0; i < n; ++i) {
        auto addr = reinterpret_cast<std::uintptr_t>(targets[i]);
        if (addr % alignof(Counter) || (addr % line_bytes) + sizeof(Counter) > line_bytes)
            throw std::runtime_error("Counter is misaligned or straddles a line");
        if (mode == "packed" && addr - base != i * sizeof(Counter))
            throw std::runtime_error("Packed stride verification failed");
        if (mode == "padded" && (addr % line_bytes || addr - base != i * line_bytes))
            throw std::runtime_error("Padded alignment verification failed");
        lines.insert(addr / line_bytes);
        if (describe) std::cerr << "layout mode=" << mode << " threads=" << n
            << " worker=" << i << " cpu=" << cpus[i] << " offset_bytes=" << addr-base
            << " line_group=" << (addr-base)/line_bytes << '\n';
    }
    const std::size_t expected_lines = mode == "padded" ? n :
        (mode == "packed" ? (n + per_line - 1) / per_line : 1);
    if (lines.size() != expected_lines) throw std::runtime_error("Line count mismatch");

    auto results = std::make_unique<ThreadResult[]>(n);
    Flag ready, go;
    std::vector<std::thread> workers;
    workers.reserve(n);
    for (int i = 0; i < n; ++i) workers.emplace_back([&, i, target=targets[i]] {
        cpu_set_t mask; CPU_ZERO(&mask); CPU_SET(cpus[i], &mask);
        results[i].pin_error = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
        ready.value.fetch_add(1, std::memory_order_release);
        ready.value.notify_one();
        go.value.wait(0, std::memory_order_acquire);
        // Same hot loop in every mode. No I/O, allocation, locks or checks here.
        for (std::uint64_t j = 0; j < iterations; ++j)
            target->fetch_add(1, std::memory_order_relaxed);
        results[i].end = Clock::now();
    });
    int seen;
    while ((seen = ready.value.load(std::memory_order_acquire)) != n)
        ready.value.wait(seen, std::memory_order_acquire);
    const auto start = Clock::now();
    go.value.store(1, std::memory_order_release);
    go.value.notify_all();
    for (auto& worker : workers) worker.join();
    auto end = start;
    for (int i = 0; i < n; ++i) {
        if (results[i].pin_error) throw std::runtime_error("Worker CPU pinning failed");
        end = std::max(end, results[i].end);
    }
    std::uint64_t sum = 0;
    if (mode == "true_shared") sum = shared.value.load(std::memory_order_relaxed);
    else for (auto* target : targets) {
        auto value = target->load(std::memory_order_relaxed);
        if (value != iterations) throw std::runtime_error("Per-thread counter mismatch");
        sum += value;
    }
    if (sum != iterations * static_cast<std::uint64_t>(n))
        throw std::runtime_error("Total count mismatch");
    return {std::chrono::duration<double>(end-start).count(), sum, lines.size()};
}

int main(int argc, char** argv) try {
    if (argc > 5 || (argc > 1 && std::string(argv[1]) == "--help")) {
        std::cerr << "Usage: " << argv[0]
                  << " [iterations=10000000] [repeats=7] [counts=auto] [mode=all]\n";
        return argc > 5 ? 1 : 0;
    }
    const auto iterations = argc > 1 ? number(argv[1]) : 10000000;
    auto reps_arg = argc > 2 ? number(argv[2]) : 7;
    if (reps_arg > 10000) throw std::runtime_error("Too many repeats");
    int repeats = static_cast<int>(reps_arg);
    auto cpus = allowed_cpus();
    std::vector<int> counts;
    if (argc > 3 && std::string(argv[3]) != "auto") {
        std::stringstream input(argv[3]); std::string part;
        while (std::getline(input, part, ',')) {
            auto count = number(part);
            if (count > cpus.size()) throw std::runtime_error("Thread count exceeds allowed CPUs");
            counts.push_back(static_cast<int>(count));
        }
    } else {
        for (std::size_t n = 1; n <= cpus.size(); n *= 2) counts.push_back(n);
        if (counts.back() != static_cast<int>(cpus.size())) counts.push_back(cpus.size());
    }
    if (counts.empty()) throw std::runtime_error("No thread counts");
    std::sort(counts.begin(), counts.end());
    counts.erase(std::unique(counts.begin(), counts.end()), counts.end());
    if (iterations > std::numeric_limits<std::uint64_t>::max() / counts.back())
        throw std::runtime_error("Total operations would overflow");
    std::vector<std::string> modes{"packed", "padded", "true_shared"};
    if (argc > 4 && std::string(argv[4]) != "all") {
        auto it = std::find(modes.begin(), modes.end(), argv[4]);
        if (it == modes.end()) throw std::runtime_error("Unknown mode");
        modes = {*it};
    }
    std::ifstream line_file("/sys/devices/system/cpu/cpu" + std::to_string(cpus[0]) +
                            "/cache/index0/coherency_line_size");
    std::size_t detected_line = 0; line_file >> detected_line;
    if (detected_line && detected_line != line_bytes)
        throw std::runtime_error("Cache line differs: rebuild with -DCACHE_LINE_BYTES=<detected>");
    std::cerr << "compiler=" << __VERSION__ << " cache_line_bytes=" << line_bytes
              << " atomic_size=" << sizeof(Counter) << " padded_stride=" << sizeof(PaddedCounter)
              << " allowed_cpus=" << cpu_string(cpus, cpus.size()) << " seed=20261008\n";
    std::cerr << "Timing excludes construction/join, includes gate wakeup and end timestamp.\n";
    std::cerr << "Warmup: min(iterations,200000) for each mode/count.\n";
    for (int n : counts) for (const auto& mode : modes)
        run(mode, n, std::min<std::uint64_t>(iterations, 200000), cpus, true);
    std::mt19937 rng(20261008);
    std::cout << "sequence,repeat,mode,threads,iterations_per_thread,total_operations,elapsed_seconds,operations_per_second,ns_per_operation,checksum,correct,cache_line_bytes,counter_stride_bytes,active_cache_lines,cpu_ids,cgroup_throttled_usec_delta\n";
    int sequence = 0;
    for (int repeat = 1; repeat <= repeats; ++repeat) {
        std::vector<std::pair<int,std::string>> jobs;
        for (int n : counts) for (const auto& mode : modes) jobs.emplace_back(n, mode);
        std::shuffle(jobs.begin(), jobs.end(), rng);
        for (const auto& [n, mode] : jobs) {
            auto throttle_before = throttled_us();
            auto measurement = run(mode, n, iterations, cpus, false);
            auto throttle_after = throttled_us();
            auto total = iterations * static_cast<std::uint64_t>(n);
            auto rate = total / measurement.seconds;
            auto delta = throttle_before >= 0 && throttle_after >= 0 ? throttle_after-throttle_before : -1;
            auto stride = mode == "padded" ? line_bytes : (mode == "packed" ? sizeof(Counter) : 0);
            std::cout << ++sequence << ',' << repeat << ',' << mode << ',' << n << ','
                      << iterations << ',' << total << ',' << std::setprecision(12)
                      << measurement.seconds << ',' << rate << ',' << 1e9/rate << ','
                      << measurement.sum << ",1," << line_bytes << ',' << stride << ','
                      << measurement.lines << ',' << cpu_string(cpus,n) << ',' << delta << '\n';
            std::cout.flush();
            std::cerr << "completed " << sequence << '/' << repeats*counts.size()*modes.size()
                      << " mode=" << mode << " threads=" << n << " seconds="
                      << measurement.seconds << '\n';
        }
    }
    return 0;
} catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << '\n'; return 1; }
