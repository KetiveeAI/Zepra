/**
 * @file SharedArrayBufferAPI.h
 * @brief SharedArrayBuffer Implementation
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <atomic>
#include <mutex>

namespace Zepra::Runtime {

// =============================================================================
// SharedArrayBuffer
// =============================================================================

class SharedArrayBuffer {
public:
    explicit SharedArrayBuffer(size_t byteLength)
        : byteLength_(byteLength), maxByteLength_(byteLength), growable_(false) {
        data_ = std::make_shared<std::vector<uint8_t>>(byteLength, 0);
    }
    
    SharedArrayBuffer(size_t byteLength, size_t maxByteLength)
        : byteLength_(byteLength), maxByteLength_(maxByteLength), growable_(true) {
        data_ = std::make_shared<std::vector<uint8_t>>(byteLength, 0);
        data_->reserve(maxByteLength);
    }
    
    size_t byteLength() const { return byteLength_.load(); }
    size_t maxByteLength() const { return maxByteLength_; }
    bool growable() const { return growable_; }
    
    uint8_t* data() { return data_->data(); }
    const uint8_t* data() const { return data_->data(); }
    
    bool grow(size_t newLength) {
        if (!growable_) return false;
        if (newLength > maxByteLength_) return false;
        if (newLength <= byteLength_.load()) return false;
        
        std::lock_guard<std::mutex> lock(mutex_);
        data_->resize(newLength, 0);
        byteLength_.store(newLength);
        return true;
    }
    
    std::shared_ptr<SharedArrayBuffer> slice(size_t begin = 0) const {
        return slice(begin, byteLength_.load());
    }
    
    std::shared_ptr<SharedArrayBuffer> slice(size_t begin, size_t end) const {
        size_t len = byteLength_.load();
        if (begin > len) begin = len;
        if (end > len) end = len;
        if (begin > end) begin = end;
        
        size_t newLen = end - begin;
        auto result = std::make_shared<SharedArrayBuffer>(newLen);
        std::memcpy(result->data(), data_->data() + begin, newLen);
        return result;
    }
    
    // Atomic access methods for Atomics API
    template<typename T>
    T atomicLoad(size_t byteOffset) const {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return T{};
        
        auto* ptr = reinterpret_cast<const std::atomic<T>*>(data_->data() + byteOffset);
        return ptr->load(std::memory_order_seq_cst);
    }
    
    template<typename T>
    void atomicStore(size_t byteOffset, T value) {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return;
        
        auto* ptr = reinterpret_cast<std::atomic<T>*>(data_->data() + byteOffset);
        ptr->store(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicExchange(size_t byteOffset, T value) {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return T{};
        
        auto* ptr = reinterpret_cast<std::atomic<T>*>(data_->data() + byteOffset);
        return ptr->exchange(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicAdd(size_t byteOffset, T value) {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return T{};
        
        auto* ptr = reinterpret_cast<std::atomic<T>*>(data_->data() + byteOffset);
        return ptr->fetch_add(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    T atomicSub(size_t byteOffset, T value) {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return T{};
        
        auto* ptr = reinterpret_cast<std::atomic<T>*>(data_->data() + byteOffset);
        return ptr->fetch_sub(value, std::memory_order_seq_cst);
    }
    
    template<typename T>
    bool atomicCompareExchange(size_t byteOffset, T& expected, T desired) {
        static_assert(std::is_integral_v<T>, "T must be integral");
        if (byteOffset + sizeof(T) > byteLength_.load()) return false;
        
        auto* ptr = reinterpret_cast<std::atomic<T>*>(data_->data() + byteOffset);
        return ptr->compare_exchange_strong(expected, desired, std::memory_order_seq_cst);
    }

private:
    std::shared_ptr<std::vector<uint8_t>> data_;
    std::atomic<size_t> byteLength_;
    size_t maxByteLength_;
    bool growable_;
    mutable std::mutex mutex_;
};

} // namespace Zepra::Runtime
