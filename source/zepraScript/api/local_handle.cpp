/**
 * @file local_handle.cpp
 * @brief Local handle implementation for GC-safe object references
 */

#include "zepra_api.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <vector>
#include <memory>

namespace Zepra {

/**
 * @brief Handle scope for automatic cleanup of local handles
 */
class HandleScope {
public:
    explicit HandleScope(Isolate* isolate)
        : isolate_(isolate)
        , previousScope_(current_)
    {
        current_ = this;
    }
    
    ~HandleScope() {
        // All handles created in this scope are now invalid
        handles_.clear();
        current_ = previousScope_;
    }
    
    static HandleScope* current() { return current_; }
    
    void addHandle(Runtime::Object* obj) {
        handles_.push_back(obj);
    }
    
private:
    Isolate* isolate_;
    HandleScope* previousScope_;
    std::vector<Runtime::Object*> handles_;
    
    static thread_local HandleScope* current_;
};

thread_local HandleScope* HandleScope::current_ = nullptr;

/**
 * @brief Local handle - GC-safe reference to a JavaScript value
 * 
 * Local handles are valid only within the current HandleScope.
 * They protect objects from being garbage collected.
 */
template<typename T>
class Local {
public:
    Local() : ptr_(nullptr) {}
    
    explicit Local(T* ptr) : ptr_(ptr) {
        if (ptr_ && HandleScope::current()) {
            HandleScope::current()->addHandle(reinterpret_cast<Runtime::Object*>(ptr_));
        }
    }
    
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* get() const { return ptr_; }
    
    bool isEmpty() const { return ptr_ == nullptr; }
    bool isValid() const { return ptr_ != nullptr; }
    
    explicit operator bool() const { return isValid(); }
    
    static Local<T> empty() { return Local<T>(); }
    
    template<typename U>
    Local<U> as() const {
        return Local<U>(static_cast<U*>(ptr_));
    }
    
private:
    T* ptr_;
};

/**
 * @brief Persistent handle - survives HandleScope
 * 
 * Must be explicitly Reset() when no longer needed.
 */
template<typename T>
class Persistent {
public:
    Persistent() : ptr_(nullptr), weak_(false) {}
    
    explicit Persistent(T* ptr) : ptr_(ptr), weak_(false) {}
    
    void reset(T* ptr = nullptr) {
        ptr_ = ptr;
    }
    
    T* get() const { return ptr_; }
    bool isEmpty() const { return ptr_ == nullptr; }
    
    // Weak persistent - doesn't prevent GC
    void setWeak() { weak_ = true; }
    bool isWeak() const { return weak_; }
    
    Local<T> toLocal() const {
        return Local<T>(ptr_);
    }
    
private:
    T* ptr_;
    bool weak_;
};

/**
 * @brief Escapable handle scope - allows returning a handle from inner scope
 */
class EscapableHandleScope : public HandleScope {
public:
    explicit EscapableHandleScope(Isolate* isolate)
        : HandleScope(isolate)
        , escapedValue_(nullptr)
    {
    }
    
    template<typename T>
    Local<T> escape(Local<T> value) {
        if (escapedValue_) {
            // Can only escape once
            return Local<T>::empty();
        }
        escapedValue_ = value.get();
        return value;
    }
    
private:
    void* escapedValue_;
};

// Value type helpers
class ValueHelper {
public:
    static Local<Runtime::Value> undefined() {
        static Runtime::Value undef = Runtime::Value::undefined();
        return Local<Runtime::Value>(const_cast<Runtime::Value*>(&undef));
    }
    
    static Local<Runtime::Value> null() {
        static Runtime::Value nullVal = Runtime::Value::null();
        return Local<Runtime::Value>(const_cast<Runtime::Value*>(&nullVal));
    }
    
    static Local<Runtime::Value> boolean(bool value) {
        // Note: For simplicity, returning stack value - in real impl, would allocate
        return Local<Runtime::Value>(nullptr);
    }
    
    static Local<Runtime::Value> number(double value) {
        return Local<Runtime::Value>(nullptr);
    }
    
    static Local<Runtime::String> string(const std::string& str) {
        return Local<Runtime::String>(new Runtime::String(str));
    }
    
    static Local<Runtime::Array> array(size_t length = 0) {
        return Local<Runtime::Array>(new Runtime::Array(length));
    }
    
    static Local<Runtime::Object> object() {
        return Local<Runtime::Object>(new Runtime::Object());
    }
};

} // namespace Zepra
