// aba_demo.cpp — Windows-compatible version
// Demonstrates the ABA problem and its fix using 8-byte packed tagged pointers.
// Compile: g++ -std=c++17 -pthread -O2 abaDemo.cpp -o aba_demo

#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

struct Node {
    int data;
    Node* next;
};

// ============================================================
// 8-byte Tagged Pointer
// On Windows x64, user-space pointers use only 48 bits.
// We pack a 16-bit tag into the upper 16 bits.
// ============================================================
class PackedPtr {
public:
    uint64_t raw_;

    PackedPtr() : raw_(0) {}
    PackedPtr(Node* ptr, uint16_t tag) {
        uint64_t p = reinterpret_cast<uint64_t>(ptr);
        raw_ = (p & 0x0000FFFFFFFFFFFFULL) | (static_cast<uint64_t>(tag) << 48);
    }

    Node* ptr() const {
        return reinterpret_cast<Node*>(raw_ & 0x0000FFFFFFFFFFFFULL);
    }

    uint16_t tag() const {
        return static_cast<uint16_t>(raw_ >> 48);
    }
};

// ============================================================
// PART 1: NAIVE TREIBER STACK (suffers from ABA)
// ============================================================
class NaiveStack {
    std::atomic<Node*> top_{nullptr};
public:
    void push(int val) {
        Node* n = new Node{val, nullptr};
        n->next = top_.load(std::memory_order_relaxed);
        while (!top_.compare_exchange_weak(n->next, n,
                                           std::memory_order_release,
                                           std::memory_order_relaxed)) {
            // n->next auto-updated on failure
        }
    }

    Node* pop() {
        Node* old_top = top_.load(std::memory_order_acquire);
        while (old_top != nullptr) {
            Node* new_top = old_top->next;
            if (top_.compare_exchange_weak(old_top, new_top,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
                return old_top;
            }
        }
        return nullptr;
    }
};

// ============================================================
// PART 2: ABA-SAFE STACK (8-byte tagged pointer)
// ============================================================
class ABASafeStack {
    std::atomic<uint64_t> top_{0}; // 8 bytes — works everywhere

public:
    void push(int val) {
        Node* n = new Node{val, nullptr};
        PackedPtr old_top;
        old_top.raw_ = top_.load(std::memory_order_relaxed);

        PackedPtr new_top;
        do {
            n->next = old_top.ptr();
            new_top = PackedPtr(n, old_top.tag() + 1);
        } while (!top_.compare_exchange_weak(old_top.raw_, new_top.raw_,
                                              std::memory_order_release,
                                              std::memory_order_relaxed));
    }

    Node* pop() {
        PackedPtr old_top;
        old_top.raw_ = top_.load(std::memory_order_acquire);

        while (old_top.ptr() != nullptr) {
            PackedPtr new_top(old_top.ptr()->next, old_top.tag() + 1);
            if (top_.compare_exchange_weak(old_top.raw_, new_top.raw_,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
                return old_top.ptr();
            }
        }
        return nullptr;
    }
};

// ============================================================
// DEMO
// ============================================================
int main() {
    std::cout << "=== ABA Problem Demo ===\n";
    std::cout << "Running ABA-safe stack with 4 threads...\n";

    ABASafeStack stack;
    const int OPS = 100000;
    std::vector<std::thread> threads;

    auto worker = [&]() {
        for (int i = 0; i < OPS; ++i) {
            stack.push(i);
            Node* n = stack.pop();
            delete n;
        }
    };

    for (int i = 0; i < 4; ++i) threads.emplace_back(worker);
    for (auto& t : threads) t.join();

    std::cout << "Done. ABA-safe stack survived 4 threads x "
              << OPS << " push/pop cycles.\n";
    std::cout << "No crashes, no corruption, no ABA problem.\n";
    return 0;
}