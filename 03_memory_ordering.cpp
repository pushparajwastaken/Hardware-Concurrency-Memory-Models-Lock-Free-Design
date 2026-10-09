/*
 * Concept : Memory ordering, data races and atomics
 * Overview: Shows a data race losing updates on a plain counter, a store-buffer
 *           litmus test where relaxed ordering lets both threads read stale values
 *           while seq_cst does not, and the release/acquire pattern for safely
 *           publishing data from one thread to another.
 *
 * Note    : The plain counter in the data race demo is intentionally unsafe.
 */

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

struct alignas(64) Cell {
    atomic<int> value{0};
};

void dataRaceDemo() {
    const int threads = 8;
    const int perThread = 1000000;

    volatile long plainCounter = 0;
    atomic<long> atomicCounter{0};

    vector<thread> pool;
    for (int i = 0; i < threads; ++i)
        pool.emplace_back([&] {
            for (int k = 0; k < perThread; ++k) {
                plainCounter = plainCounter + 1;
                atomicCounter.fetch_add(1, memory_order_relaxed);
            }
        });
    for (auto& t : pool) t.join();

    long expected = (long)threads * perThread;
    cout << "[1] Data race\n";
    cout << "    expected       : " << expected << "\n";
    cout << "    plain counter  : " << plainCounter << " (lost " << expected - plainCounter << " updates)\n";
    cout << "    atomic counter : " << atomicCounter.load() << "\n";
}

template <memory_order Order>
long litmusTest(int rounds) {
    Cell x, y;
    int r1 = 0, r2 = 0;
    atomic<int> arrived{0};
    long bothZero = 0;

    auto barrier = [&](int step) {
        arrived.fetch_add(1);
        while (arrived.load() < 2 * step) {
        }
    };

    thread other([&] {
        int step = 0;
        for (int i = 0; i < rounds; ++i) {
            barrier(++step);
            y.value.store(1, Order);
            r2 = x.value.load(Order);
            barrier(++step);
        }
    });

    int step = 0;
    for (int i = 0; i < rounds; ++i) {
        x.value.store(0, memory_order_relaxed);
        y.value.store(0, memory_order_relaxed);
        barrier(++step);
        x.value.store(1, Order);
        r1 = y.value.load(Order);
        barrier(++step);
        if (r1 == 0 && r2 == 0) ++bothZero;
    }

    other.join();
    return bothZero;
}

void litmusDemo() {
    const int rounds = 500000;
    cout << "\n[2] Store-buffer litmus test (" << rounds << " rounds)\n";
    cout << "    outcome r1 == 0 and r2 == 0 means both loads ran before the other store was visible\n";
    cout << "    relaxed : " << litmusTest<memory_order_relaxed>(rounds) << " reorderings\n";
    cout << "    seq_cst : " << litmusTest<memory_order_seq_cst>(rounds) << " reorderings\n";
}

void releaseAcquireDemo() {
    int payload = 0;
    atomic<bool> ready{false};

    thread producer([&] {
        payload = 42;
        ready.store(true, memory_order_release);
    });

    thread consumer([&] {
        while (!ready.load(memory_order_acquire)) {
        }
        cout << "\n[3] Release/acquire\n";
        cout << "    consumer read payload = " << payload << "\n";
    });

    producer.join();
    consumer.join();
}

int main() {
    dataRaceDemo();
    litmusDemo();
    releaseAcquireDemo();
    return 0;
}
