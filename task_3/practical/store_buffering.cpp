#include <atomic>
#include <barrier>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[]) {
    int iterations = 100000;
    bool seq_cst = false;

    if (argc > 1) {
        iterations = std::atoi(argv[1]);
    }

    if (argc > 2) {
        seq_cst = std::string(argv[2]) == "seq_cst";
    }

    if (iterations <= 0) {
        std::cerr << "Iterations must be positive.\n";
        return 1;
    }

    std::atomic<int> X{0}, Y{0};
    int r1 = 0, r2 = 0, both_zero = 0;
    std::barrier sync(3);

    auto worker1 = [&]() {
        for (int i = 0; i < iterations; ++i) {
            sync.arrive_and_wait();

            if (seq_cst) {
                X.store(1, std::memory_order_seq_cst);
                r1 = Y.load(std::memory_order_seq_cst);
            } else {
                X.store(1, std::memory_order_relaxed);
                r1 = Y.load(std::memory_order_relaxed);
            }

            sync.arrive_and_wait();
        }
    };

    auto worker2 = [&]() {
        for (int i = 0; i < iterations; ++i) {
            sync.arrive_and_wait();

            if (seq_cst) {
                Y.store(1, std::memory_order_seq_cst);
                r2 = X.load(std::memory_order_seq_cst);
            } else {
                Y.store(1, std::memory_order_relaxed);
                r2 = X.load(std::memory_order_relaxed);
            }

            sync.arrive_and_wait();
        }
    };

    std::thread t1(worker1), t2(worker2);

    for (int i = 0; i < iterations; ++i) {
        X.store(0, std::memory_order_relaxed);
        Y.store(0, std::memory_order_relaxed);

        sync.arrive_and_wait();
        sync.arrive_and_wait();

        if (r1 == 0 && r2 == 0) {
            ++both_zero;
        }
    }

    t1.join();
    t2.join();

    std::cout << "Mode: "
              << (seq_cst ? "seq_cst" : "relaxed") << '\n';
    std::cout << "Iterations: " << iterations << '\n';
    std::cout << "Both-zero outcomes: " << both_zero << '\n';
}
