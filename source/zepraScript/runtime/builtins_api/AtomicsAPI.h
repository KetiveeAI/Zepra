/**
 * @file AtomicsAPI.h
 * @brief Atomics Implementation
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace Zepra::Runtime {

class Atomics {
public:
    // Arithmetic
    template<typename T>
    static T add(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->fetch_add(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    static T sub(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->fetch_sub(value, std::memory_order_seq_cst);
    }
    
    // Bitwise
    template<typename T>
    static T and_(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->fetch_and(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    static T or_(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->fetch_or(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    static T xor_(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->fetch_xor(value, std::memory_order_seq_cst);
    }
    
    // Load/Store
    template<typename T>
    static T load(T* typedArray, size_t index) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->load(std::memory_order_seq_cst);
    }
    
    template<typename T>
    static T store(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        atomic->store(value, std::memory_order_seq_cst);
        return value;
    }
    
    // Exchange
    template<typename T>
    static T exchange(T* typedArray, size_t index, T value) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        return atomic->exchange(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    static T compareExchange(T* typedArray, size_t index, T expectedValue, T replacementValue) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        atomic->compare_exchange_strong(expectedValue, replacementValue, std::memory_order_seq_cst);
        return expectedValue;
    }
    
    // Wait/Notify
    enum class WaitResult { Ok, NotEqual, TimedOut };
    
    template<typename T>
    static WaitResult wait(T* typedArray, size_t index, T value, int64_t timeoutMs = -1) {
        auto* atomic = reinterpret_cast<std::atomic<T>*>(&typedArray[index]);
        
        if (atomic->load() != value) {
            return WaitResult::NotEqual;
        }
        
        std::unique_lock<std::mutex> lock(waitMutex());
        auto& cv = getCondVar(typedArray, index);
        
        if (timeoutMs < 0) {
            cv.wait(lock, [&]() { return atomic->load() != value; });
            return WaitResult::Ok;
        } else {
            auto result = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                       [&]() { return atomic->load() != value; });
            return result ? WaitResult::Ok : WaitResult::TimedOut;
        }
    }
    
    template<typename T>
    static int notify(T* typedArray, size_t index, int count = 1) {
        std::lock_guard<std::mutex> lock(waitMutex());
        auto& cv = getCondVar(typedArray, index);
        
        if (count == INT_MAX) {
            cv.notify_all();
            return count;
        }
        
        for (int i = 0; i < count; ++i) {
            cv.notify_one();
        }
        return count;
    }
    
    // isLockFree
    template<typename T>
    static bool isLockFree() {
        return std::atomic<T>::is_always_lock_free;
    }

private:
    static std::mutex& waitMutex() {
        static std::mutex mutex;
        return mutex;
    }
    
    template<typename T>
    static std::condition_variable& getCondVar(T* typedArray, size_t index) {
        static std::unordered_map<uintptr_t, std::condition_variable> cvMap;
        uintptr_t key = reinterpret_cast<uintptr_t>(&typedArray[index]);
        return cvMap[key];
    }
};

} // namespace Zepra::Runtime
