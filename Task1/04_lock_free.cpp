/*
 * Concept : Lock-free design (atomic CAS, lock-free stack, ring buffer)
 * Overview: Compares a CAS-loop counter with a mutex counter, implements a
 *           lock-free Treiber stack, and builds a bounded single-producer
 *           single-consumer ring buffer with cache-line padded indices,
 *           benchmarked against a mutex-protected queue.
 *
 * Note    : The stack never frees popped nodes, which keeps the demo simple and
 *           avoids the ABA problem. Real code needs hazard pointers or epochs.
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

using Clock = steady_clock;

const size_t CACHE_LINE = 64;

double elapsedMs(Clock::time_point start) {
    return duration<double, milli>(Clock::now() - start).count();
}

template <typename T>
class LockFreeStack {
    struct Node {
        T value;
        Node* below;
    };
    atomic<Node*> head{nullptr};

public:
    void push(T value) {
        Node* node = new Node{value, head.load(memory_order_relaxed)};
        while (!head.compare_exchange_weak(node->below, node, memory_order_release, memory_order_relaxed)) {
        }
    }

    bool pop(T& out) {
        Node* top = head.load(memory_order_acquire);
        while (top && !head.compare_exchange_weak(top, top->below, memory_order_acquire, memory_order_acquire)) {
        }
        if (!top) return false;
        out = top->value;
        return true;
    }
};

template <typename T, size_t N>
class SpscRingBuffer {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
    alignas(CACHE_LINE) atomic<size_t> readIndex{0};
    alignas(CACHE_LINE) atomic<size_t> writeIndex{0};
    alignas(CACHE_LINE) T slots[N];

public:
    bool push(const T& value) {
        size_t w = writeIndex.load(memory_order_relaxed);
        if (w - readIndex.load(memory_order_acquire) == N) return false;
        slots[w & (N - 1)] = value;
        writeIndex.store(w + 1, memory_order_release);
        return true;
    }

    bool pop(T& value) {
        size_t r = readIndex.load(memory_order_relaxed);
        if (r == writeIndex.load(memory_order_acquire)) return false;
        value = slots[r & (N - 1)];
        readIndex.store(r + 1, memory_order_release);
        return true;
    }
};

class MutexQueue {
    mutex lock;
    queue<uint64_t> items;
    size_t capacity;

public:
    explicit MutexQueue(size_t cap) : capacity(cap) {}

    bool push(uint64_t value) {
        lock_guard<mutex> guard(lock);
        if (items.size() == capacity) return false;
        items.push(value);
        return true;
    }

    bool pop(uint64_t& value) {
        lock_guard<mutex> guard(lock);
        if (items.empty()) return false;
        value = items.front();
        items.pop();
        return true;
    }
};

void casVsMutex() {
    const unsigned threads = 4;
    const int perThread = 2000000;

    atomic<long> casCounter{0};
    auto start = Clock::now();
    {
        vector<thread> pool;
        for (unsigned i = 0; i < threads; ++i)
            pool.emplace_back([&] {
                for (int k = 0; k < perThread; ++k) {
                    long expected = casCounter.load(memory_order_relaxed);
                    while (!casCounter.compare_exchange_weak(expected, expected + 1, memory_order_relaxed)) {
                    }
                }
            });
        for (auto& t : pool) t.join();
    }
    double casMs = elapsedMs(start);

    long mutexCounter = 0;
    mutex lock;
    start = Clock::now();
    {
        vector<thread> pool;
        for (unsigned i = 0; i < threads; ++i)
            pool.emplace_back([&] {
                for (int k = 0; k < perThread; ++k) {
                    lock_guard<mutex> guard(lock);
                    ++mutexCounter;
                }
            });
        for (auto& t : pool) t.join();
    }
    double mutexMs = elapsedMs(start);

    cout << "[1] CAS loop vs mutex (" << threads << " threads x " << perThread << " increments)\n";
    cout << "    CAS loop : " << fixed << setprecision(1) << casMs << " ms (result " << casCounter << ")\n";
    cout << "    mutex    : " << mutexMs << " ms (result " << mutexCounter << ")\n";
}

void stackTest() {
    LockFreeStack<int> stack;
    const int threads = 4;
    const int perThread = 100000;

    vector<thread> pool;
    for (int i = 0; i < threads; ++i)
        pool.emplace_back([&] {
            for (int k = 1; k <= perThread; ++k) stack.push(k);
        });
    for (auto& t : pool) t.join();

    atomic<long long> sum{0};
    atomic<int> count{0};
    pool.clear();
    for (int i = 0; i < threads; ++i)
        pool.emplace_back([&] {
            int value;
            while (stack.pop(value)) {
                sum += value;
                ++count;
            }
        });
    for (auto& t : pool) t.join();

    long long expected = (long long)threads * perThread * (perThread + 1) / 2;
    cout << "\n[2] Lock-free stack\n";
    cout << "    pushed " << threads * perThread << ", popped " << count << "\n";
    cout << "    sum " << sum << " (expected " << expected << ") -> "
         << (sum == expected && count == threads * perThread ? "CORRECT" : "BUG") << "\n";
}

template <typename Queue>
double runPipeline(Queue& queue, uint64_t items, bool& correct) {
    uint64_t sum = 0;
    auto start = Clock::now();

    thread producer([&] {
        for (uint64_t i = 1; i <= items; ++i)
            while (!queue.push(i)) {
            }
    });
    thread consumer([&] {
        uint64_t value, received = 0;
        while (received < items)
            if (queue.pop(value)) {
                sum += value;
                ++received;
            }
    });

    producer.join();
    consumer.join();
    correct = (sum == items * (items + 1) / 2);
    return elapsedMs(start);
}

void ringBufferTest() {
    const uint64_t items = 10000000;
    bool ringOk = false, mutexOk = false;

    SpscRingBuffer<uint64_t, 1024>* ring = new SpscRingBuffer<uint64_t, 1024>();
    MutexQueue mutexQueue(1024);

    double ringMs = runPipeline(*ring, items, ringOk);
    double mutexMs = runPipeline(mutexQueue, items, mutexOk);
    delete ring;

    cout << "\n[3] Producer -> consumer, " << items << " items, capacity 1024\n";
    cout << "    lock-free ring buffer : " << fixed << setprecision(1) << ringMs << " ms ("
         << items / ringMs / 1000.0 << " M items/s) " << (ringOk ? "checksum OK" : "BUG") << "\n";
    cout << "    mutex queue           : " << mutexMs << " ms ("
         << items / mutexMs / 1000.0 << " M items/s) " << (mutexOk ? "checksum OK" : "BUG") << "\n";
    cout << "    speed-up              : " << setprecision(2) << mutexMs / ringMs << "x\n";
}

int main() {
    casVsMutex();
    stackTest();
    ringBufferTest();
    return 0;
}
