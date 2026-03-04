/**
 * @file WasmHostBindings.h
 * @brief WASM ↔ ZepraScript Host Bindings Layer
 * 
 * Implements:
 * - HostFunction wrapper for JS functions callable from WASM
 * - WasmValue ↔ JSValue conversion
 * - Callback registration and invocation
 * - Exception propagation
 */

#pragma once

#include "wasm.hpp"
#include <functional>
#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

namespace Zepra::Wasm {

// Forward declarations
class WasmInstance;

// =============================================================================
// WASM Value Types
// =============================================================================

/**
 * @brief Boxed WASM value for host interop
 */
class WasmValue {
public:
    using ValueVariant = std::variant<
        int32_t,    // i32
        int64_t,    // i64
        float,      // f32
        double,     // f64
        void*,      // externref/funcref
        std::nullptr_t  // null
    >;
    
    WasmValue() : value_(nullptr) {}
    WasmValue(int32_t v) : value_(v), type_(ValType::i32()) {}
    WasmValue(int64_t v) : value_(v), type_(ValType::i64()) {}
    WasmValue(float v) : value_(v), type_(ValType::f32()) {}
    WasmValue(double v) : value_(v), type_(ValType::f64()) {}
    WasmValue(void* v) : value_(v), type_(ValType::externRef()) {}
    
    // Type checking
    bool isI32() const { return std::holds_alternative<int32_t>(value_); }
    bool isI64() const { return std::holds_alternative<int64_t>(value_); }
    bool isF32() const { return std::holds_alternative<float>(value_); }
    bool isF64() const { return std::holds_alternative<double>(value_); }
    bool isRef() const { return std::holds_alternative<void*>(value_); }
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    
    // Value extraction
    int32_t asI32() const { return std::get<int32_t>(value_); }
    int64_t asI64() const { return std::get<int64_t>(value_); }
    float asF32() const { return std::get<float>(value_); }
    double asF64() const { return std::get<double>(value_); }
    void* asRef() const { return std::get<void*>(value_); }
    
    ValType type() const { return type_; }
    
private:
    ValueVariant value_;
    ValType type_;
};

// =============================================================================
// Host Function Interface
// =============================================================================

/**
 * @brief Host function signature
 */
using HostFunctionCallback = std::function<
    std::vector<WasmValue>(WasmInstance*, const std::vector<WasmValue>&)
>;

/**
 * @brief Host function descriptor
 */
struct HostFunctionDesc {
    std::string name;
    std::vector<ValType> paramTypes;
    std::vector<ValType> resultTypes;
    HostFunctionCallback callback;
};

/**
 * @brief Host function wrapper
 */
class HostFunction {
public:
    HostFunction(std::string name, 
                 std::vector<ValType> params,
                 std::vector<ValType> results,
                 HostFunctionCallback callback)
        : name_(std::move(name))
        , paramTypes_(std::move(params))
        , resultTypes_(std::move(results))
        , callback_(std::move(callback)) {}
    
    const std::string& name() const { return name_; }
    const std::vector<ValType>& paramTypes() const { return paramTypes_; }
    const std::vector<ValType>& resultTypes() const { return resultTypes_; }
    
    // Invoke the host function
    std::vector<WasmValue> invoke(WasmInstance* instance, 
                                  const std::vector<WasmValue>& args) {
        // Validate argument count
        if (args.size() != paramTypes_.size()) {
            throw std::runtime_error("Argument count mismatch");
        }
        
        // Validate argument types
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i].type().kind != paramTypes_[i].kind) {
                throw std::runtime_error("Argument type mismatch at index " + std::to_string(i));
            }
        }
        
        // Invoke callback
        return callback_(instance, args);
    }
    
    // Generate trampoline code offset
    uintptr_t trampolineOffset() const { return trampolineOffset_; }
    void setTrampolineOffset(uintptr_t offset) { trampolineOffset_ = offset; }
    
private:
    std::string name_;
    std::vector<ValType> paramTypes_;
    std::vector<ValType> resultTypes_;
    HostFunctionCallback callback_;
    uintptr_t trampolineOffset_ = 0;
};

// =============================================================================
// externref Registry
// =============================================================================

/**
 * @brief Handle to an external reference
 */
using ExternRefHandle = uint32_t;

/**
 * @brief Registry for tracking externref values
 */
class ExternRefRegistry {
public:
    // Register a new externref, returns handle
    ExternRefHandle registerRef(void* value) {
        ExternRefHandle handle = nextHandle_++;
        refs_[handle] = {value, 1};
        return handle;
    }
    
    // Look up externref by handle
    void* getRef(ExternRefHandle handle) const {
        auto it = refs_.find(handle);
        return it != refs_.end() ? it->second.value : nullptr;
    }
    
    // Increment reference count
    void addRef(ExternRefHandle handle) {
        auto it = refs_.find(handle);
        if (it != refs_.end()) {
            it->second.refCount++;
        }
    }
    
    // Decrement reference count, returns true if freed
    bool releaseRef(ExternRefHandle handle) {
        auto it = refs_.find(handle);
        if (it != refs_.end()) {
            it->second.refCount--;
            if (it->second.refCount == 0) {
                refs_.erase(it);
                return true;
            }
        }
        return false;
    }
    
    // Get all live handles (for GC root scanning)
    std::vector<void*> getAllRefs() const {
        std::vector<void*> result;
        for (const auto& [handle, entry] : refs_) {
            result.push_back(entry.value);
        }
        return result;
    }
    
    // Clear all refs (for cleanup)
    void clear() {
        refs_.clear();
        nextHandle_ = 1;
    }
    
private:
    struct RefEntry {
        void* value;
        uint32_t refCount;
    };
    
    std::unordered_map<ExternRefHandle, RefEntry> refs_;
    ExternRefHandle nextHandle_ = 1;
};

// =============================================================================
// Host Bindings Manager
// =============================================================================

/**
 * @brief WASM exception for host-side errors
 */
class WasmHostException : public std::exception {
public:
    WasmHostException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }
    
private:
    std::string message_;
};

/**
 * @brief Manages host bindings for a WASM instance
 */
class HostBindingsManager {
public:
    // Register a host function
    void registerFunction(const std::string& module, 
                         const std::string& name,
                         std::vector<ValType> params,
                         std::vector<ValType> results,
                         HostFunctionCallback callback) {
        std::string key = module + "." + name;
        functions_[key] = std::make_shared<HostFunction>(
            name, std::move(params), std::move(results), std::move(callback)
        );
    }
    
    // Register a host memory
    void registerMemory(const std::string& module,
                       const std::string& name,
                       void* memory, size_t size) {
        std::string key = module + "." + name;
        memories_[key] = {memory, size};
    }
    
    // Register a host global
    void registerGlobal(const std::string& module,
                       const std::string& name,
                       WasmValue value, bool mutable_) {
        std::string key = module + "." + name;
        globals_[key] = {value, mutable_};
    }
    
    // Lookup function
    std::shared_ptr<HostFunction> getFunction(const std::string& module,
                                              const std::string& name) const {
        std::string key = module + "." + name;
        auto it = functions_.find(key);
        return it != functions_.end() ? it->second : nullptr;
    }
    
    // Lookup memory
    std::pair<void*, size_t> getMemory(const std::string& module,
                                       const std::string& name) const {
        std::string key = module + "." + name;
        auto it = memories_.find(key);
        return it != memories_.end() ? it->second : std::make_pair(nullptr, 0UL);
    }
    
    // externref registry
    ExternRefRegistry& externRefs() { return externRefs_; }
    const ExternRefRegistry& externRefs() const { return externRefs_; }
    
    // Invoke a host function from WASM
    std::vector<WasmValue> invokeHost(WasmInstance* instance,
                                      const std::string& module,
                                      const std::string& name,
                                      const std::vector<WasmValue>& args) {
        auto func = getFunction(module, name);
        if (!func) {
            throw WasmHostException("Host function not found: " + module + "." + name);
        }
        
        try {
            return func->invoke(instance, args);
        } catch (const std::exception& e) {
            // Propagate exception to WASM
            throw WasmHostException(std::string("Host function error: ") + e.what());
        }
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<HostFunction>> functions_;
    std::unordered_map<std::string, std::pair<void*, size_t>> memories_;
    std::unordered_map<std::string, std::pair<WasmValue, bool>> globals_;
    ExternRefRegistry externRefs_;
};

// =============================================================================
// Value Conversion Helpers
// =============================================================================

namespace Convert {

// Convert WASM integer to JS number
inline double wasmToNumber(const WasmValue& v) {
    if (v.isI32()) return static_cast<double>(v.asI32());
    if (v.isI64()) return static_cast<double>(v.asI64());
    if (v.isF32()) return static_cast<double>(v.asF32());
    if (v.isF64()) return v.asF64();
    return 0.0;
}

// Convert JS number to WASM i32
inline int32_t numberToI32(double n) {
    return static_cast<int32_t>(n);
}

// Convert JS number to WASM i64
inline int64_t numberToI64(double n) {
    return static_cast<int64_t>(n);
}

// Convert string to WASM memory (returns pointer in linear memory)
inline uint32_t stringToMemory(void* memory, uint32_t offset, 
                               const std::string& str, size_t maxLen) {
    size_t len = std::min(str.size(), maxLen - 1);
    std::memcpy(static_cast<char*>(memory) + offset, str.c_str(), len);
    static_cast<char*>(memory)[offset + len] = '\0';
    return static_cast<uint32_t>(len);
}

// Convert WASM memory to string
inline std::string memoryToString(const void* memory, uint32_t offset, uint32_t len) {
    return std::string(static_cast<const char*>(memory) + offset, len);
}

} // namespace Convert

} // namespace Zepra::Wasm
