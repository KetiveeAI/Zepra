/**
 * @file DisposableAPI.h
 * @brief Disposable Resources API (ES2024)
 */

#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Disposable Interface
// =============================================================================

class Disposable {
public:
    virtual ~Disposable() = default;
    virtual void dispose() = 0;
};

class AsyncDisposable {
public:
    virtual ~AsyncDisposable() = default;
    virtual void asyncDispose(std::function<void()> callback) = 0;
};

// =============================================================================
// DisposableStack
// =============================================================================

class DisposableStack {
public:
    DisposableStack() = default;
    ~DisposableStack() { dispose(); }
    
    DisposableStack(const DisposableStack&) = delete;
    DisposableStack& operator=(const DisposableStack&) = delete;
    DisposableStack(DisposableStack&&) = default;
    DisposableStack& operator=(DisposableStack&&) = default;
    
    bool disposed() const { return disposed_; }
    
    void use(std::shared_ptr<Disposable> value) {
        if (disposed_) throw std::runtime_error("DisposableStack already disposed");
        stack_.push_back([value]() { value->dispose(); });
    }
    
    void adopt(std::shared_ptr<void> value, std::function<void(std::shared_ptr<void>)> onDispose) {
        if (disposed_) throw std::runtime_error("DisposableStack already disposed");
        stack_.push_back([value, onDispose]() { onDispose(value); });
    }
    
    void defer(std::function<void()> onDispose) {
        if (disposed_) throw std::runtime_error("DisposableStack already disposed");
        stack_.push_back(onDispose);
    }
    
    void dispose() {
        if (disposed_) return;
        disposed_ = true;
        
        std::exception_ptr firstException;
        for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
            try {
                (*it)();
            } catch (...) {
                if (!firstException) firstException = std::current_exception();
            }
        }
        stack_.clear();
        
        if (firstException) std::rethrow_exception(firstException);
    }
    
    DisposableStack move() {
        DisposableStack result;
        std::swap(result.stack_, stack_);
        disposed_ = true;
        return result;
    }

private:
    std::vector<std::function<void()>> stack_;
    bool disposed_ = false;
};

// =============================================================================
// AsyncDisposableStack
// =============================================================================

class AsyncDisposableStack {
public:
    AsyncDisposableStack() = default;
    
    bool disposed() const { return disposed_; }
    
    void use(std::shared_ptr<AsyncDisposable> value) {
        if (disposed_) throw std::runtime_error("AsyncDisposableStack already disposed");
        stack_.push_back([value](std::function<void()> done) {
            value->asyncDispose(done);
        });
    }
    
    void defer(std::function<void(std::function<void()>)> onDispose) {
        if (disposed_) throw std::runtime_error("AsyncDisposableStack already disposed");
        stack_.push_back(onDispose);
    }
    
    void disposeAsync(std::function<void()> callback) {
        if (disposed_) {
            callback();
            return;
        }
        disposed_ = true;
        disposeNext(0, callback);
    }

private:
    void disposeNext(size_t index, std::function<void()> finalCallback) {
        if (index >= stack_.size()) {
            stack_.clear();
            finalCallback();
            return;
        }
        
        size_t revIndex = stack_.size() - 1 - index;
        stack_[revIndex]([this, index, finalCallback]() {
            disposeNext(index + 1, finalCallback);
        });
    }
    
    std::vector<std::function<void(std::function<void()>)>> stack_;
    bool disposed_ = false;
};

// =============================================================================
// SuppressedError
// =============================================================================

class SuppressedError : public std::runtime_error {
public:
    SuppressedError(std::exception_ptr error, std::exception_ptr suppressed)
        : std::runtime_error("SuppressedError"), error_(error), suppressed_(suppressed) {}
    
    std::exception_ptr error() const { return error_; }
    std::exception_ptr suppressed() const { return suppressed_; }

private:
    std::exception_ptr error_;
    std::exception_ptr suppressed_;
};

} // namespace Zepra::Runtime
