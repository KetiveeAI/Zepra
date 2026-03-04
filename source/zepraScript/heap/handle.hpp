#pragma once

/**
 * @file handle.hpp
 * @brief GC-safe handles for preventing premature collection
 */

#include "../config.hpp"
#include "runtime/objects/value.hpp"

namespace Zepra::GC {

class Heap;

/**
 * @brief Local handle scope
 * 
 * Handles created within this scope are automatically
 * cleaned up when the scope exits.
 */
class HandleScope {
public:
    explicit HandleScope(Heap* heap);
    ~HandleScope();
    
    HandleScope(const HandleScope&) = delete;
    HandleScope& operator=(const HandleScope&) = delete;
    
private:
    Heap* heap_;
    size_t previousHandleCount_;
};

/**
 * @brief Handle to a GC-managed value
 * 
 * Prevents the garbage collector from collecting the value.
 */
template<typename T>
class Handle {
public:
    Handle() : ptr_(nullptr) {}
    explicit Handle(T* ptr) : ptr_(ptr) {}
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
    bool operator==(const Handle& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const Handle& other) const { return ptr_ != other.ptr_; }
    
private:
    T* ptr_;
};

/**
 * @brief Persistent handle that survives across handle scopes
 */
template<typename T>
class Persistent {
public:
    Persistent() : ptr_(nullptr), registered_(false) {}
    Persistent(Heap* heap, T* ptr);
    ~Persistent();
    
    Persistent(const Persistent&) = delete;
    Persistent& operator=(const Persistent&) = delete;
    
    Persistent(Persistent&& other) noexcept;
    Persistent& operator=(Persistent&& other) noexcept;
    
    void reset();
    void reset(T* ptr);
    
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
private:
    Heap* heap_ = nullptr;
    T* ptr_;
    bool registered_;
};

} // namespace Zepra::GC
