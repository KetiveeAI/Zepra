/**
 * @file AsyncContextAPI.h
 * @brief AsyncContext Implementation
 */

#pragma once

#include <functional>
#include <memory>
#include <any>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace Zepra::Runtime {

// =============================================================================
// AsyncContext Variable
// =============================================================================

template<typename T>
class AsyncContextVariable {
public:
    explicit AsyncContextVariable(T defaultValue = T{}) : defaultValue_(std::move(defaultValue)) {}
    
    T get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = storage_.find(std::this_thread::get_id());
        if (it != storage_.end() && !it->second.empty()) {
            return it->second.back();
        }
        return defaultValue_;
    }
    
    template<typename F>
    auto run(T value, F&& fn) -> decltype(fn()) {
        push(value);
        try {
            auto result = fn();
            pop();
            return result;
        } catch (...) {
            pop();
            throw;
        }
    }

private:
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        storage_[std::this_thread::get_id()].push_back(value);
    }
    
    void pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& stack = storage_[std::this_thread::get_id()];
        if (!stack.empty()) stack.pop_back();
    }
    
    T defaultValue_;
    mutable std::mutex mutex_;
    std::unordered_map<std::thread::id, std::vector<T>> storage_;
};

// =============================================================================
// AsyncContext Snapshot
// =============================================================================

class AsyncContextSnapshot {
public:
    using Restorer = std::function<void()>;
    
    void capture(std::function<std::any()> getter, std::function<void(std::any)> setter) {
        entries_.push_back({getter(), setter});
    }
    
    template<typename F>
    auto run(F&& fn) -> decltype(fn()) {
        std::vector<std::any> saved;
        for (auto& entry : entries_) {
            saved.push_back(entry.value);
            entry.setter(entry.value);
        }
        
        try {
            auto result = fn();
            return result;
        } catch (...) {
            throw;
        }
    }

private:
    struct Entry {
        std::any value;
        std::function<void(std::any)> setter;
    };
    std::vector<Entry> entries_;
};

// =============================================================================
// AsyncContext
// =============================================================================

class AsyncContext {
public:
    template<typename T>
    using Variable = AsyncContextVariable<T>;
    using Snapshot = AsyncContextSnapshot;
    
    static Snapshot snapshot() {
        Snapshot snap;
        return snap;
    }
    
    template<typename T>
    static Variable<T> createVariable(T defaultValue = T{}) {
        return Variable<T>(std::move(defaultValue));
    }
};

// =============================================================================
// Async Local Storage (Node.js style)
// =============================================================================

template<typename T>
class AsyncLocalStorage {
public:
    T getStore() const { return variable_.get(); }
    
    template<typename F>
    auto run(T store, F&& fn) -> decltype(fn()) {
        return variable_.run(store, std::forward<F>(fn));
    }
    
    template<typename F>
    auto enterWith(T store, F&& fn) -> decltype(fn()) {
        return run(store, std::forward<F>(fn));
    }
    
    void disable() { disabled_ = true; }
    bool enabled() const { return !disabled_; }

private:
    AsyncContextVariable<T> variable_;
    bool disabled_ = false;
};

} // namespace Zepra::Runtime
