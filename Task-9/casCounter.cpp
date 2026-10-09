#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <fstream>
#include <chrono>

std::atomic<long long> counter(0);
std::atomic<long long> total_retries(0);

void increment_counter(int iterations) {
    long long local_retries = 0;
    for (int i = 0; i < iterations; ++i) {
        long long expected = counter.load(std::memory_order_relaxed);
        long long desired = expected + 1;
        while (!counter.compare_exchange_weak(expected, desired,
                                              std::memory_order_seq_cst,
                                              std::memory_order_relaxed)) {
            desired = expected + 1;
            local_retries++;
        }
    }
    total_retries.fetch_add(local_retries, std::memory_order_relaxed);
}

int main() {
    const int NUM_THREADS = 8;
    const int ITERATIONS = 1000000;
    std::cout << "Starting CAS Counter Benchmark...\n";
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i)
        threads.emplace_back(increment_counter, ITERATIONS);
    for (auto& t : threads) t.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    long long expected_final = (long long)NUM_THREADS * ITERATIONS;
    std::cout << "Expected: " << expected_final << " | Actual: " << counter.load()
              << " | Retries: " << total_retries.load() << " | Time: " << duration << "ms\n";
    
//     std::ofstream file("cas-results.csv");
//     file << "Threads,Iterations,Expected,Actual,TotalRetries,TimeMs\n";
//     file << NUM_THREADS << "," << ITERATIONS << "," << expected_final << ","
//          << counter.load() << "," << total_retries.load() << "," << duration << "\n";
//     file.close();
//     return 0;
// }
    // ── cas-results.csv generation ──
    std::ofstream file("cas-results.csv", std::ios::app); // Append mode
    if (file.is_open()) {
        // Check if file is empty, then write header
        file.seekp(0, std::ios::end);
        if (file.tellp() == 0) {
            file << "RunID,Threads,IterationsPerThread,ExpectedCounter,ActualCounter,TotalRetries,TimeMs,Observation\n";
        }
        file << "1," << NUM_THREADS << "," << ITERATIONS << "," 
             << expected_final << "," << counter.load() << "," 
             << total_retries.load() << "," << duration 
             << ",\"Manually run on my hardware\"\n";
        file.close();
        std::cout << "Data appended to cas-results.csv\n";
    }
}