#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "lockFreeRingBuffer.hpp"

const int OPERATIONS = 1000000;
const int QUEUE_SIZE = 1024;

// --- Mutex-based Queue for Comparison ---
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

void run_mutex_benchmark() {
    mutex_queue = std::queue<int>(); // clear queue for each run
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread p(mutex_producer);
    std::thread c(mutex_consumer);
    p.join(); c.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double ops_per_sec = (double)(OPERATIONS * 2) / (duration_ms / 1000.0);
    double latency_ns = (double)duration_ms * 1e6 / (OPERATIONS * 2);
    
    std::cout << "[Mutex]           | Time: " << duration_ms << " ms"
              << " | Throughput: " << (long long)ops_per_sec << " ops/sec"
              << " | Avg Latency: " << latency_ns << " ns/op\n";
}

// --- Lock-Free SPSC Ring Buffer Benchmark ---
void run_lockfree_benchmark() {
    LockFreeRingBuffer<int, QUEUE_SIZE> lf_queue;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Producer: SIRF 1 thread
    auto producer_fn = [&]() {
        for (int i = 0; i < OPERATIONS; ++i) {
            while (!lf_queue.push(i)) {
                std::this_thread::yield(); // Yield to avoid OS starvation
            }
        }
    };
    
    // Consumer: SIRF 1 thread
    auto consumer_fn = [&]() {
        int val;
        for (int i = 0; i < OPERATIONS; ++i) {
            while (!lf_queue.pop(val)) {
                std::this_thread::yield(); // Yield to avoid OS starvation
            }
        }
    };
    
    std::thread p(producer_fn);
    std::thread c(consumer_fn);
    p.join(); c.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double ops_per_sec = (double)(OPERATIONS * 2) / (duration_ms / 1000.0);
    double latency_ns = (double)duration_ms * 1e6 / (OPERATIONS * 2);
    
    std::cout << "[Lock-Free SPSC]  | Time: " << duration_ms << " ms"
              << " | Throughput: " << (long long)ops_per_sec << " ops/sec"
              << " | Avg Latency: " << latency_ns << " ns/op\n";
}

int main() {
    std::cout << "=== Benchmark: Mutex vs Lock-Free (SPSC) Ring Buffer ===\n";
    std::cout << "Operations: " << OPERATIONS << " | Queue Size: " << QUEUE_SIZE << "\n\n";
    
    run_mutex_benchmark();
    std::cout << "------------------------------------------------\n";
    run_lockfree_benchmark();
    
    return 0;
}