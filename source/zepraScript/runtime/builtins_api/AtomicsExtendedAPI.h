/**
 * @file AtomicsExtendedAPI.h
 * @brief Atomics wait/notify Implementation
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>
#include <future>

namespace Zepra::Runtime {

// =============================================================================
// Wait Result
// =============================================================================

enum class WaitResult { Ok, NotEqual, TimedOut };

// =============================================================================
// Atomics Wait/Notify Manager
// =============================================================================

class AtomicsWaitManager {
public:
    static AtomicsWaitManager& instance() {
        static AtomicsWaitManager manager;
        return manager;
    }
    
    template<typename T>
    WaitResult wait(std::atomic<T>* ptr, T expected, int64_t timeoutMs = -1) {
        if (ptr->load() != expected) {
            return WaitResult::NotEqual;
        }
        
        auto key = reinterpret_cast<uintptr_t>(ptr);
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto& entry = waiters_[key];
        entry.count++;
        
        bool result;
        if (timeoutMs < 0) {
            entry.cv.wait(lock, [&]() {
                return ptr->load() != expected || entry.notified;
            });
            result = true;
        } else {
            result = entry.cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&]() {
                return ptr->load() != expected || entry.notified;
            });
        }
        
        entry.count--;
        entry.notified = false;
        
        if (entry.count == 0) {
            waiters_.erase(key);
        }
        
        return result ? WaitResult::Ok : WaitResult::TimedOut;
    }
    
    template<typename T>
    size_t notify(std::atomic<T>* ptr, size_t count = 1) {
        auto key = reinterpret_cast<uintptr_t>(ptr);
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = waiters_.find(key);
        if (it == waiters_.end()) return 0;
        
        size_t awakened = std::min(count, static_cast<size_t>(it->second.count));
        it->second.notified = true;
        
        if (count == 1) {
            it->second.cv.notify_one();
        } else {
            it->second.cv.notify_all();
        }
        
        return awakened;
    }
    
    template<typename T>
    size_t notifyAll(std::atomic<T>* ptr) {
        return notify(ptr, SIZE_MAX);
    }

private:
    struct WaitEntry {
        std::condition_variable cv;
        int count = 0;
        bool notified = false;
    };
    
    std::mutex mutex_;
    std::unordered_map<uintptr_t, WaitEntry> waiters_;
};

// =============================================================================
// Atomics Extended Functions
// =============================================================================

namespace Atomics {

template<typename T>
WaitResult wait(std::atomic<T>* ptr, T expected, int64_t timeoutMs = -1) {
    return AtomicsWaitManager::instance().wait(ptr, expected, timeoutMs);
}

template<typename T>
size_t notify(std::atomic<T>* ptr, size_t count = 1) {
    return AtomicsWaitManager::instance().notify(ptr, count);
}

template<typename T>
size_t notifyAll(std::atomic<T>* ptr) {
    return AtomicsWaitManager::instance().notifyAll(ptr);
}

// Async wait returns a future
template<typename T>
std::future<WaitResult> waitAsync(std::atomic<T>* ptr, T expected, int64_t timeoutMs = -1) {
    return std::async(std::launch::async, [=]() {
        return wait(ptr, expected, timeoutMs);
    });
}

// Fence operations
inline void fence() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

inline void pause() {
    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        __asm__ __volatile__("pause");
    #elif defined(__aarch64__) || defined(_M_ARM64)
        __asm__ __volatile__("yield");
    #else
        std::this_thread::yield();
    #endif
}

} // namespace Atomics

} // namespace Zepra::Runtime
