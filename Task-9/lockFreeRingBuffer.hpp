#pragma once
#include <atomic>
#include <vector>
#include <cstddef>

template<typename T, size_t Capacity>
class LockFreeRingBuffer {
private:
    // Align head and tail to separate cache lines to prevent false sharing
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    char padding_[64]; // Prevent data buffer from sharing cache line with pointers
    std::vector<T> buffer_;

public:
    LockFreeRingBuffer() : buffer_(Capacity) {}

    // Producer thread calls this
    bool push(const T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;

        // Check if full (next tail catches up to head)
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }

        buffer_[current_tail] = item;
        // Release: Ensure data write happens before tail update is visible
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Consumer thread calls this
    bool pop(T& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);

        // Check if empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }

        item = buffer_[current_head];
        // Release: Ensure data read happens before head update is visible
        head_.store((current_head + 1) % Capacity, std::memory_order_release);
        return true;
    }
};