//=== Benchmark: Mutex vs Lock-Free (SPSC) Ring Buffer ===
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include "lockFreeRingBuffer.hpp"

const int OPERATIONS = 1000000;
const int QUEUE_SIZE = 1024;

// --- Mutex-based Queue ---
std::mutex mtx;
std::condition_variable cv;
std::queue<int> mutex_queue;

void mutex_producer() {
    for (int i = 0; i < OPERATIONS; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        while (mutex_queue.size() >= QUEUE_SIZE) cv.wait(lock);
        mutex_queue.push(i);
        cv.notify_one();
    }
}

void mutex_consumer() {
    for (int i = 0; i < OPERATIONS; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        while (mutex_queue.empty()) cv.wait(lock);
        mutex_queue.pop();
        cv.notify_one();
    }
}

int main() {
    std::cout << "=== Benchmark: Mutex vs Lock-Free (SPSC) Ring Buffer ===\n";
    std::cout << "Operations: " << OPERATIONS << " | Queue Size: " << QUEUE_SIZE << "\n\n";

    // Open CSV file for writing
    std::ofstream csv("ringBufferBench.csv");
    csv << "Mode,Time_ms,Throughput_ops_sec,Avg_Latency_ns\n";

    // ── 1. MUTEX BENCHMARK ──
    auto start_mutex = std::chrono::high_resolution_clock::now();
    std::thread mp(mutex_producer);
    std::thread mc(mutex_consumer);
    mp.join(); mc.join();
    auto end_mutex = std::chrono::high_resolution_clock::now();

    auto duration_mutex = std::chrono::duration_cast<std::chrono::milliseconds>(end_mutex - start_mutex).count();
    double throughput_mutex = (double)(OPERATIONS * 2) / (duration_mutex / 1000.0);
    double latency_mutex = (double)duration_mutex * 1e6 / (OPERATIONS * 2);

    std::cout << "[Mutex]          | Time: " << duration_mutex << " ms"
              << " | Throughput: " << (long long)throughput_mutex << " ops/sec"
              << " | Avg Latency: " << latency_mutex << " ns/op\n";
    csv << "Mutex," << duration_mutex << "," << (long long)throughput_mutex << "," << latency_mutex << "\n";

    // ── 2. LOCK-FREE BENCHMARK ──
    LockFreeRingBuffer<int, QUEUE_SIZE> lf_queue;
    auto start_lf = std::chrono::high_resolution_clock::now();

    auto lf_producer = [&]() {
        for (int i = 0; i < OPERATIONS; ++i) {
            while (!lf_queue.push(i)) { std::this_thread::yield(); }
        }
    };

    auto lf_consumer = [&]() {
        int val;
        for (int i = 0; i < OPERATIONS; ++i) {
            while (!lf_queue.pop(val)) { std::this_thread::yield(); }
        }
    };

    std::thread lp(lf_producer);
    std::thread lc(lf_consumer);
    lp.join(); lc.join();
    auto end_lf = std::chrono::high_resolution_clock::now();

    auto duration_lf = std::chrono::duration_cast<std::chrono::milliseconds>(end_lf - start_lf).count();
    double throughput_lf = (double)(OPERATIONS * 2) / (duration_lf / 1000.0);
    double latency_lf = (double)duration_lf * 1e6 / (OPERATIONS * 2);

    std::cout << "[Lock-Free SPSC] | Time: " << duration_lf << " ms"
              << " | Throughput: " << (long long)throughput_lf << " ops/sec"
              << " | Avg Latency: " << latency_lf << " ns/op\n";
    csv << "Lock-Free SPSC," << duration_lf << "," << (long long)throughput_lf << "," << latency_lf << "\n";

    csv.close();
    std::cout << "\nData written to ringBufferBench.csv\n";
    return 0;
}