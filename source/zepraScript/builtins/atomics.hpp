/**
 * @file atomics.hpp
 * @brief Atomics and SharedArrayBuffer (ES2017)
 * 
 * Provides atomic operations for concurrent access to shared memory.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "typed_array.hpp"
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Builtins {

/**
 * @brief SharedArrayBuffer - shareable between workers
 */
class SharedArrayBufferObject : public Runtime::Object {
public:
    explicit SharedArrayBufferObject(size_t byteLength);
    ~SharedArrayBufferObject();
    
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    size_t byteLength() const { return byteLength_; }
    
    // Reference counting for sharing
    void addRef() { refCount_++; }
    void release();
    
    // Grow (ES2024)
    bool grow(size_t newByteLength);
    bool growable() const { return maxByteLength_ > byteLength_; }
    size_t maxByteLength() const { return maxByteLength_; }
    
private:
    uint8_t* data_;
    size_t byteLength_;
    size_t maxByteLength_;
    std::atomic<int> refCount_{1};
};

/**
 * @brief Wait entry for Atomics.wait
 */
struct WaitEntry {
    std::mutex mutex;
    std::condition_variable cv;
    bool notified = false;
};

/**
 * @brief Atomics namespace - atomic operations
 */
class AtomicsBuiltin {
public:
    // Atomic operations
    static Runtime::Value add(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value and_(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value compareExchange(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value exchange(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value isLockFree(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value load(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value or_(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value store(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value sub(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value xor_(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Wait/notify
    static Runtime::Value wait(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value waitAsync(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    static Runtime::Value notify(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // SharedArrayBuffer constructor
    static Runtime::Value sharedArrayBufferConstructor(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Create Atomics object
    static Runtime::Object* createAtomicsObject(Runtime::Context* ctx);
    static void registerGlobal(Runtime::Object* global);
    
private:
    // Validate typed array for atomics
    static bool validateTypedArray(const Runtime::Value& arg, TypedArrayObject*& out);
    static bool validateSharedArray(const Runtime::Value& arg, TypedArrayObject*& out);
    
    // Wait list management
    static std::mutex waitListMutex_;
    static std::unordered_map<void*, std::vector<WaitEntry*>> waitLists_;
};

} // namespace Zepra::Builtins
