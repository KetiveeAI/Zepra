/**
 * @file concurrent_queue.h
 * @brief Lock-free MPMC concurrent queue
 */

#pragma once

#include <atomic>
#include <vector>
#include <optional>
#include <cstddef>
#include <new>

namespace Zepra::Threading {

/**
 * Bounded lock-free multi-producer multi-consumer queue.
 * Uses cache-line padded ring buffer with per-slot sequence counters.
 *
 * Ref: Dmitry Vyukov's bounded MPMC queue
 */
template<typename T>
class ConcurrentQueue {
public:
    explicit ConcurrentQueue(size_t capacity)
        : capacity_(nextPowerOf2(capacity))
        , mask_(capacity_ - 1)
        , buffer_(capacity_)
        , head_(0)
        , tail_(0) {
        for (size_t i = 0; i < capacity_; ++i) {
            buffer_[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    bool tryPush(const T& item) {
        size_t pos = head_.load(std::memory_order_relaxed);

        for (;;) {
            Slot& slot = buffer_[pos & mask_];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            if (diff == 0) {
                if (head_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed)) {
                    slot.data = item;
                    slot.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false; // Queue full
            } else {
                pos = head_.load(std::memory_order_relaxed);
            }
        }
    }

    bool tryPush(T&& item) {
        size_t pos = head_.load(std::memory_order_relaxed);

        for (;;) {
            Slot& slot = buffer_[pos & mask_];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

            if (diff == 0) {
                if (head_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed)) {
                    slot.data = std::move(item);
                    slot.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = head_.load(std::memory_order_relaxed);
            }
        }
    }

    std::optional<T> tryPop() {
        size_t pos = tail_.load(std::memory_order_relaxed);

        for (;;) {
            Slot& slot = buffer_[pos & mask_];
            size_t seq = slot.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) -
                            static_cast<intptr_t>(pos + 1);

            if (diff == 0) {
                if (tail_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed)) {
                    T result = std::move(slot.data);
                    slot.seq.store(pos + capacity_, std::memory_order_release);
                    return result;
                }
            } else if (diff < 0) {
                return std::nullopt; // Queue empty
            } else {
                pos = tail_.load(std::memory_order_relaxed);
            }
        }
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) ==
               tail_.load(std::memory_order_relaxed);
    }

    size_t capacity() const { return capacity_; }

    size_t size() const {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_relaxed);
        return (h >= t) ? (h - t) : 0;
    }

private:
    static size_t nextPowerOf2(size_t n) {
        n--;
        n |= n >> 1; n |= n >> 2; n |= n >> 4;
        n |= n >> 8; n |= n >> 16; n |= n >> 32;
        return n + 1;
    }

    struct alignas(64) Slot {
        std::atomic<size_t> seq;
        T data;
    };

    size_t capacity_;
    size_t mask_;
    std::vector<Slot> buffer_;

    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace Zepra::Threading
