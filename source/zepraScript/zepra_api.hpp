#pragma once

/**
 * @file zepra_api.hpp
 * @brief Main public API for embedding ZepraScript in host applications
 * 
 * This is the primary header for applications using ZepraScript.
 * Include this header to access all public API functionality.
 */

#include "config.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <optional>
#include <variant>

namespace Zepra {

// Forward declarations
class Isolate;
class Context;
class Value;
class Object;
class Function;
class String;
class Array;
class Script;
class Module;

/**
 * @brief Initialize the ZepraScript engine
 * 
 * Must be called before any other ZepraScript API calls.
 * Safe to call multiple times; subsequent calls are no-ops.
 * 
 * @return true if initialization succeeded
 */
ZEPRA_API bool initialize();

/**
 * @brief Shutdown the ZepraScript engine
 * 
 * Releases all global resources. After calling this,
 * initialize() must be called again to use the engine.
 */
ZEPRA_API void shutdown();

/**
 * @brief Get the ZepraScript version string
 */
ZEPRA_API const char* getVersion();

/**
 * @brief Result type for operations that may fail
 */
template<typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(std::string error) : data_(std::move(error)) {}
    
    bool isSuccess() const { return std::holds_alternative<T>(data_); }
    bool isError() const { return std::holds_alternative<std::string>(data_); }
    
    const T& value() const { return std::get<T>(data_); }
    T& value() { return std::get<T>(data_); }
    
    const std::string& error() const { return std::get<std::string>(data_); }
    
    explicit operator bool() const { return isSuccess(); }
    
private:
    std::variant<T, std::string> data_;
};

/**
 * @brief Exception types in ZepraScript
 */
enum class ErrorType {
    SyntaxError,
    TypeError,
    ReferenceError,
    RangeError,
    EvalError,
    URIError,
    InternalError
};

/**
 * @brief Represents a JavaScript exception
 */
class ZEPRA_API Exception {
public:
    Exception(ErrorType type, std::string message, std::string stack = "");
    
    ErrorType type() const { return type_; }
    const std::string& message() const { return message_; }
    const std::string& stack() const { return stack_; }
    
    std::string toString() const;
    
private:
    ErrorType type_;
    std::string message_;
    std::string stack_;
};

/**
 * @brief Options for creating an Isolate
 */
struct IsolateOptions {
    size_t initialHeapSize = ZEPRA_GC_INITIAL_HEAP_SIZE;
    size_t maxHeapSize = ZEPRA_GC_MAX_HEAP_SIZE;
    size_t maxCallStackDepth = ZEPRA_MAX_CALL_STACK_DEPTH;
    bool enableJit = ZEPRA_ENABLE_JIT;
    bool enableDebug = ZEPRA_ENABLE_DEBUG;
};

/**
 * @brief An isolated JavaScript execution environment
 * 
 * Each Isolate has its own heap, garbage collector, and
 * can have multiple Contexts for script execution.
 */
class ZEPRA_API Isolate {
public:
    /**
     * @brief Create a new Isolate
     */
    static std::unique_ptr<Isolate> create(const IsolateOptions& options = {});
    
    virtual ~Isolate() = default;
    
    /**
     * @brief Create a new execution context
     */
    virtual std::unique_ptr<Context> createContext() = 0;
    
    /**
     * @brief Request garbage collection
     * @param fullGC If true, perform a full GC; otherwise incremental
     */
    virtual void collectGarbage(bool fullGC = false) = 0;
    
    /**
     * @brief Get heap statistics
     */
    struct HeapStats {
        size_t totalHeapSize;
        size_t usedHeapSize;
        size_t objectCount;
    };
    virtual HeapStats getHeapStats() const = 0;
    
protected:
    Isolate() = default;
};

/**
 * @brief A JavaScript execution context
 * 
 * Contains a global object and can execute scripts.
 */
class ZEPRA_API Context {
public:
    virtual ~Context() = default;
    
    /**
     * @brief Execute JavaScript code
     * @param source The JavaScript source code
     * @param filename Optional filename for error messages
     * @return The result value or an exception
     */
    virtual Result<Value> evaluate(std::string_view source, 
                                   std::string_view filename = "<eval>") = 0;
    
    /**
     * @brief Get the global object
     */
    virtual Object* globalObject() = 0;
    
    /**
     * @brief Get the associated Isolate
     */
    virtual Isolate* isolate() = 0;
    
protected:
    Context() = default;
};

/**
 * @brief Native function callback type
 */
using NativeCallback = std::function<Value(Context*, const std::vector<Value>&)>;

} // namespace Zepra
