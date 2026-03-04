/**
 * @file atomics.cpp
 * @brief Atomics and SharedArrayBuffer implementation (ES2017)
 */

#include "builtins/atomics.hpp"
#include "runtime/objects/function.hpp"
#include <thread>
#include <chrono>
#include <cstring>
#include <climits>

namespace Zepra::Builtins {

// Static members
std::mutex AtomicsBuiltin::waitListMutex_;
std::unordered_map<void*, std::vector<WaitEntry*>> AtomicsBuiltin::waitLists_;

// =============================================================================
// SharedArrayBufferObject
// =============================================================================

SharedArrayBufferObject::SharedArrayBufferObject(size_t byteLength)
    : Runtime::Object(Runtime::ObjectType::ArrayBuffer)
    , byteLength_(byteLength)
    , maxByteLength_(byteLength)
{
    data_ = new uint8_t[byteLength]();
}

SharedArrayBufferObject::~SharedArrayBufferObject() {
    delete[] data_;
}

void SharedArrayBufferObject::release() {
    if (--refCount_ == 0) {
        delete this;
    }
}

bool SharedArrayBufferObject::grow(size_t newByteLength) {
    if (newByteLength <= byteLength_ || newByteLength > maxByteLength_) {
        return false;
    }
    
    uint8_t* newData = new uint8_t[newByteLength]();
    std::memcpy(newData, data_, byteLength_);
    delete[] data_;
    data_ = newData;
    byteLength_ = newByteLength;
    return true;
}

// =============================================================================
// AtomicsBuiltin - Helper functions
// =============================================================================

bool AtomicsBuiltin::validateTypedArray(const Runtime::Value& arg, TypedArrayObject*& out) {
    if (!arg.isObject()) return false;
    out = dynamic_cast<TypedArrayObject*>(arg.asObject());
    return out != nullptr;
}

bool AtomicsBuiltin::validateSharedArray(const Runtime::Value& arg, TypedArrayObject*& out) {
    if (!validateTypedArray(arg, out)) return false;
    // For now, allow any typed array (SharedArrayBuffer check would go here)
    return true;
}

// =============================================================================
// AtomicsBuiltin - Atomic operations
// =============================================================================

Runtime::Value AtomicsBuiltin::add(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    // Atomic add (simplified - actual impl uses std::atomic)
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->fetch_add(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

Runtime::Value AtomicsBuiltin::and_(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->fetch_and(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

Runtime::Value AtomicsBuiltin::compareExchange(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 4) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t expected = static_cast<int32_t>(args[2].toNumber());
    int32_t replacement = static_cast<int32_t>(args[3].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    atomicPtr->compare_exchange_strong(expected, replacement, std::memory_order_seq_cst);
    
    return Runtime::Value::number(expected);
}

Runtime::Value AtomicsBuiltin::exchange(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->exchange(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

Runtime::Value AtomicsBuiltin::isLockFree(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::boolean(false);
    
    int size = static_cast<int>(args[0].toNumber());
    
    // Standard lock-free sizes
    return Runtime::Value::boolean(size == 1 || size == 2 || size == 4 || size == 8);
}

Runtime::Value AtomicsBuiltin::load(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t value = atomicPtr->load(std::memory_order_seq_cst);
    
    return Runtime::Value::number(value);
}

Runtime::Value AtomicsBuiltin::or_(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->fetch_or(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

Runtime::Value AtomicsBuiltin::store(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    atomicPtr->store(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(value);
}

Runtime::Value AtomicsBuiltin::sub(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->fetch_sub(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

Runtime::Value AtomicsBuiltin::xor_(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::undefined();
    
    TypedArrayObject* ta;
    if (!validateTypedArray(args[0], ta)) return Runtime::Value::undefined();
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t value = static_cast<int32_t>(args[2].toNumber());
    
    if (index >= ta->length()) return Runtime::Value::undefined();
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    int32_t oldValue = atomicPtr->fetch_xor(value, std::memory_order_seq_cst);
    
    return Runtime::Value::number(oldValue);
}

// =============================================================================
// AtomicsBuiltin - Wait/notify
// =============================================================================

Runtime::Value AtomicsBuiltin::wait(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 3) return Runtime::Value::string(new Runtime::String("not-equal"));
    
    TypedArrayObject* ta;
    if (!validateSharedArray(args[0], ta)) {
        return Runtime::Value::string(new Runtime::String("not-equal"));
    }
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int32_t expected = static_cast<int32_t>(args[2].toNumber());
    int64_t timeout = args.size() > 3 ? static_cast<int64_t>(args[3].toNumber()) : -1;
    
    if (index >= ta->length()) {
        return Runtime::Value::string(new Runtime::String("not-equal"));
    }
    
    // Load current value
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    std::atomic<int32_t>* atomicPtr = reinterpret_cast<std::atomic<int32_t>*>(ptr);
    
    if (atomicPtr->load() != expected) {
        return Runtime::Value::string(new Runtime::String("not-equal"));
    }
    
    // Create wait entry
    WaitEntry entry;
    
    {
        std::lock_guard<std::mutex> lock(waitListMutex_);
        waitLists_[ptr].push_back(&entry);
    }
    
    // Wait
    std::unique_lock<std::mutex> waitLock(entry.mutex);
    
    if (timeout < 0) {
        entry.cv.wait(waitLock, [&] { return entry.notified; });
    } else {
        auto result = entry.cv.wait_for(waitLock, std::chrono::milliseconds(timeout),
                                        [&] { return entry.notified; });
        if (!result) {
            // Remove from wait list
            std::lock_guard<std::mutex> lock(waitListMutex_);
            auto& list = waitLists_[ptr];
            list.erase(std::remove(list.begin(), list.end(), &entry), list.end());
            return Runtime::Value::string(new Runtime::String("timed-out"));
        }
    }
    
    return Runtime::Value::string(new Runtime::String("ok"));
}

Runtime::Value AtomicsBuiltin::waitAsync(Runtime::Context*, const std::vector<Runtime::Value>&) {
    // Returns a Promise that resolves when notified
    // Simplified implementation - actual impl would create a Promise
    return Runtime::Value::undefined();
}

Runtime::Value AtomicsBuiltin::notify(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::number(0);
    
    TypedArrayObject* ta;
    if (!validateSharedArray(args[0], ta)) {
        return Runtime::Value::number(0);
    }
    
    size_t index = static_cast<size_t>(args[1].toNumber());
    int count = args.size() > 2 ? static_cast<int>(args[2].toNumber()) : INT_MAX;
    
    if (index >= ta->length()) return Runtime::Value::number(0);
    
    int32_t* ptr = reinterpret_cast<int32_t*>(ta->buffer()->data() + ta->byteOffset() + index * 4);
    
    int woken = 0;
    
    {
        std::lock_guard<std::mutex> lock(waitListMutex_);
        auto it = waitLists_.find(ptr);
        if (it != waitLists_.end()) {
            auto& list = it->second;
            while (!list.empty() && woken < count) {
                WaitEntry* entry = list.front();
                list.erase(list.begin());
                
                {
                    std::lock_guard<std::mutex> entryLock(entry->mutex);
                    entry->notified = true;
                }
                entry->cv.notify_one();
                woken++;
            }
        }
    }
    
    return Runtime::Value::number(woken);
}

// =============================================================================
// AtomicsBuiltin - Constructor and registration
// =============================================================================

Runtime::Value AtomicsBuiltin::sharedArrayBufferConstructor(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    size_t byteLength = args.empty() ? 0 : static_cast<size_t>(args[0].toNumber());
    return Runtime::Value::object(new SharedArrayBufferObject(byteLength));
}

Runtime::Object* AtomicsBuiltin::createAtomicsObject(Runtime::Context*) {
    Runtime::Object* atomics = new Runtime::Object();
    
    atomics->set("add", Runtime::Value::object(
        new Runtime::Function("add", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::add(nullptr, args);
        }, 3)));
    
    atomics->set("and", Runtime::Value::object(
        new Runtime::Function("and", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::and_(nullptr, args);
        }, 3)));
    
    atomics->set("compareExchange", Runtime::Value::object(
        new Runtime::Function("compareExchange", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::compareExchange(nullptr, args);
        }, 4)));
    
    atomics->set("exchange", Runtime::Value::object(
        new Runtime::Function("exchange", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::exchange(nullptr, args);
        }, 3)));
    
    atomics->set("isLockFree", Runtime::Value::object(
        new Runtime::Function("isLockFree", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::isLockFree(nullptr, args);
        }, 1)));
    
    atomics->set("load", Runtime::Value::object(
        new Runtime::Function("load", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::load(nullptr, args);
        }, 2)));
    
    atomics->set("or", Runtime::Value::object(
        new Runtime::Function("or", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::or_(nullptr, args);
        }, 3)));
    
    atomics->set("store", Runtime::Value::object(
        new Runtime::Function("store", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::store(nullptr, args);
        }, 3)));
    
    atomics->set("sub", Runtime::Value::object(
        new Runtime::Function("sub", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::sub(nullptr, args);
        }, 3)));
    
    atomics->set("wait", Runtime::Value::object(
        new Runtime::Function("wait", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::wait(nullptr, args);
        }, 4)));
    
    atomics->set("notify", Runtime::Value::object(
        new Runtime::Function("notify", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::notify(nullptr, args);
        }, 3)));
    
    atomics->set("xor", Runtime::Value::object(
        new Runtime::Function("xor", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::xor_(nullptr, args);
        }, 3)));
    
    return atomics;
}

void AtomicsBuiltin::registerGlobal(Runtime::Object* global) {
    global->set("Atomics", Runtime::Value::object(createAtomicsObject(nullptr)));
    global->set("SharedArrayBuffer", Runtime::Value::object(
        new Runtime::Function("SharedArrayBuffer", [](const Runtime::FunctionCallInfo& info) -> Runtime::Value {
            std::vector<Runtime::Value> args;
            for (size_t i = 0; i < info.argumentCount(); i++) args.push_back(info.argument(i));
            return AtomicsBuiltin::sharedArrayBufferConstructor(nullptr, args);
        }, 1)));
}

} // namespace Zepra::Builtins
