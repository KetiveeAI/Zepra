/**
 * @file EmbedderAPI.h
 * @brief Clean API for Browser Integration
 * 
 * V8/JSC-inspired embedder API:
 * - ZebraIsolate: Isolated VM instance
 * - ZebraContext: Execution context
 * - ZebraScript: Compiled script handle
 * - ZebraValue: Boxed JS value for C++
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <vector>
#include <optional>

namespace Zepra {

// Forward declarations
class ZebraIsolate;
class ZebraContext;
class ZebraScript;
class ZebraValue;
class ZebraObject;
class ZebraFunction;
class ZebraException;

// =============================================================================
// Callbacks
// =============================================================================

using GCCallback = std::function<void(size_t heapSize, size_t usedSize)>;
using OOMCallback = std::function<bool(size_t requestedSize)>;
using ExceptionCallback = std::function<void(const ZebraException&)>;
using MessageCallback = std::function<void(int level, std::string_view msg)>;
using PromiseRejectCallback = std::function<void(ZebraValue promise, ZebraValue reason)>;

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize the Zepra engine (call once at startup)
 */
bool ZebraInitialize();

/**
 * @brief Shutdown the Zepra engine (call before exit)
 */
void ZebraShutdown();

/**
 * @brief Get engine version string
 */
const char* ZebraVersion();

// =============================================================================
// ZebraIsolate (Isolated VM Instance)
// =============================================================================

/**
 * @brief Configuration for creating an isolate
 */
struct IsolateConfig {
    size_t maxHeapSize = 512 * 1024 * 1024;  // 512 MB
    size_t initialHeapSize = 4 * 1024 * 1024; // 4 MB
    size_t stackLimit = 1024 * 1024;          // 1 MB stack
    bool enableJIT = true;
    bool enableWasm = true;
    bool enableICU = true;
    
    // Callbacks
    GCCallback gcCallback;
    OOMCallback oomCallback;
    MessageCallback messageCallback;
};

/**
 * @brief Isolated VM instance (one per thread typically)
 */
class ZebraIsolate {
public:
    static std::unique_ptr<ZebraIsolate> Create(const IsolateConfig& config = {});
    ~ZebraIsolate();
    
    // Non-copyable
    ZebraIsolate(const ZebraIsolate&) = delete;
    ZebraIsolate& operator=(const ZebraIsolate&) = delete;
    
    // Enter/Exit isolate (RAII preferred)
    void Enter();
    void Exit();
    
    // GC control
    void RequestGC();
    void ForceGC();
    
    // Heap statistics
    struct HeapStats {
        size_t totalHeapSize;
        size_t usedHeapSize;
        size_t heapLimit;
        size_t externalMemory;
        size_t gcCount;
    };
    HeapStats GetHeapStats() const;
    
    // External memory tracking
    void AdjustExternalMemory(int64_t delta);
    
    // Terminate execution
    void TerminateExecution();
    bool IsExecutionTerminated() const;
    void CancelTerminateExecution();
    
    // Current isolate
    static ZebraIsolate* Current();
    
private:
    ZebraIsolate(const IsolateConfig& config);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Isolate Scope (RAII)
// =============================================================================

class IsolateScope {
public:
    explicit IsolateScope(ZebraIsolate* isolate) : isolate_(isolate) {
        isolate_->Enter();
    }
    ~IsolateScope() {
        isolate_->Exit();
    }
    
private:
    ZebraIsolate* isolate_;
};

// =============================================================================
// ZebraContext (Execution Context)
// =============================================================================

/**
 * @brief Execution context with global object
 */
class ZebraContext {
public:
    static std::unique_ptr<ZebraContext> Create(ZebraIsolate* isolate);
    ~ZebraContext();
    
    // Enter/Exit context
    void Enter();
    void Exit();
    
    // Global object
    ZebraObject* Global();
    
    // Security token (for cross-context access)
    void SetSecurityToken(const std::string& token);
    std::string GetSecurityToken() const;
    
    // Embedded data slots
    void SetData(uint32_t index, void* data);
    void* GetData(uint32_t index) const;
    static constexpr uint32_t kDataSlots = 32;
    
    // Current context
    static ZebraContext* Current();
    
private:
    ZebraContext(ZebraIsolate* isolate);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Context Scope (RAII)
// =============================================================================

class ContextScope {
public:
    explicit ContextScope(ZebraContext* context) : context_(context) {
        context_->Enter();
    }
    ~ContextScope() {
        context_->Exit();
    }
    
private:
    ZebraContext* context_;
};

// =============================================================================
// ZebraScript (Compiled Script)
// =============================================================================

/**
 * @brief Compile options
 */
struct CompileOptions {
    std::string filename = "<script>";
    int lineOffset = 0;
    int columnOffset = 0;
    bool isModule = false;
    bool strictMode = false;
};

/**
 * @brief Compiled script handle
 */
class ZebraScript {
public:
    static std::optional<ZebraScript> Compile(
        ZebraContext* context,
        std::string_view source,
        const CompileOptions& options = {});
    
    // Run script
    std::optional<ZebraValue> Run(ZebraContext* context);
    
    // Source info
    std::string GetSourceURL() const;
    
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

// =============================================================================
// ZebraValue (Boxed JS Value)
// =============================================================================

/**
 * @brief Boxed JavaScript value for C++ interop
 */
class ZebraValue {
public:
    ZebraValue();  // Undefined
    ~ZebraValue();
    
    // Copy/Move
    ZebraValue(const ZebraValue& other);
    ZebraValue(ZebraValue&& other) noexcept;
    ZebraValue& operator=(const ZebraValue& other);
    ZebraValue& operator=(ZebraValue&& other) noexcept;
    
    // Type checks
    bool IsUndefined() const;
    bool IsNull() const;
    bool IsBoolean() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsObject() const;
    bool IsArray() const;
    bool IsFunction() const;
    bool IsSymbol() const;
    bool IsBigInt() const;
    
    // Conversions
    bool ToBoolean() const;
    double ToNumber() const;
    int32_t ToInt32() const;
    uint32_t ToUint32() const;
    std::string ToString() const;
    ZebraObject* ToObject() const;
    
    // Factory methods
    static ZebraValue Undefined();
    static ZebraValue Null();
    static ZebraValue Boolean(bool value);
    static ZebraValue Number(double value);
    static ZebraValue String(ZebraContext* ctx, std::string_view str);
    static ZebraValue Integer(int32_t value);
    
    // Equality
    bool StrictEquals(const ZebraValue& other) const;
    bool Equals(const ZebraValue& other) const;
    
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

// =============================================================================
// ZebraObject
// =============================================================================

/**
 * @brief JavaScript object wrapper
 */
class ZebraObject {
public:
    // Property access
    bool Has(std::string_view key) const;
    std::optional<ZebraValue> Get(std::string_view key) const;
    bool Set(std::string_view key, const ZebraValue& value);
    bool Delete(std::string_view key);
    
    // Index access
    std::optional<ZebraValue> GetIndex(uint32_t index) const;
    bool SetIndex(uint32_t index, const ZebraValue& value);
    
    // Enumeration
    std::vector<std::string> GetOwnPropertyNames() const;
    
    // Internal fields (for C++ data)
    void SetInternalField(uint32_t index, void* ptr);
    void* GetInternalField(uint32_t index) const;
    
    // Create object
    static ZebraObject* Create(ZebraContext* ctx);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// ZebraFunction
// =============================================================================

using FunctionCallback = std::function<ZebraValue(
    const ZebraValue& thisArg,
    const std::vector<ZebraValue>& args)>;

/**
 * @brief JavaScript function wrapper
 */
class ZebraFunction {
public:
    // Call function
    std::optional<ZebraValue> Call(
        const ZebraValue& thisArg,
        const std::vector<ZebraValue>& args);
    
    // Construct (new)
    std::optional<ZebraObject*> NewInstance(
        const std::vector<ZebraValue>& args);
    
    // Create from C++ callback
    static ZebraFunction* Create(
        ZebraContext* ctx,
        const std::string& name,
        FunctionCallback callback,
        int length = 0);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// ZebraException
// =============================================================================

/**
 * @brief Exception information
 */
class ZebraException {
public:
    std::string Message() const;
    std::string StackTrace() const;
    
    // Source location
    std::string SourceFile() const;
    int LineNumber() const;
    int ColumnNumber() const;
    
    // Exception value
    ZebraValue GetValue() const;
    
    // Create exceptions
    static ZebraException Error(std::string_view msg);
    static ZebraException TypeError(std::string_view msg);
    static ZebraException RangeError(std::string_view msg);
    static ZebraException ReferenceError(std::string_view msg);
    static ZebraException SyntaxError(std::string_view msg);
    
private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

// =============================================================================
// Try/Catch Scope
// =============================================================================

class TryCatch {
public:
    explicit TryCatch(ZebraIsolate* isolate);
    ~TryCatch();
    
    bool HasCaught() const;
    ZebraException Exception() const;
    void Reset();
    
    // Rethrow to outer TryCatch
    void ReThrow();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Template for C++ Classes
// =============================================================================

/**
 * @brief Template for exposing C++ classes to JS
 */
template<typename T>
class ZebraClassTemplate {
public:
    using Constructor = std::function<T*(const std::vector<ZebraValue>&)>;
    using Destructor = std::function<void(T*)>;
    
    static ZebraClassTemplate<T>* Create(
        ZebraIsolate* isolate,
        const std::string& className);
    
    // Set constructor
    void SetConstructor(Constructor ctor);
    
    // Add method
    void SetMethod(const std::string& name, FunctionCallback callback);
    
    // Add property
    using Getter = std::function<ZebraValue(T*)>;
    using Setter = std::function<void(T*, const ZebraValue&)>;
    void SetProperty(const std::string& name, Getter getter, Setter setter = nullptr);
    
    // Get constructor function
    ZebraFunction* GetFunction(ZebraContext* ctx);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra
