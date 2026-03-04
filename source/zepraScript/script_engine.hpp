#pragma once

/**
 * @file script_engine.hpp
 * @brief High-level script execution interface
 */

#include "zepra_api.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace Zepra {

/**
 * @brief High-level interface for script execution
 * 
 * This class provides a simple API for executing JavaScript
 * code without needing to manage Isolates and Contexts directly.
 */
class ZEPRA_API ScriptEngine {
public:
    /**
     * @brief Create a new script engine
     */
    static std::unique_ptr<ScriptEngine> create(const IsolateOptions& options = {});
    
    virtual ~ScriptEngine() = default;
    
    /**
     * @brief Execute JavaScript code and return the result
     * @param source JavaScript source code
     * @param filename Optional filename for error reporting
     * @return Result of execution or error message
     */
    virtual Result<Value> execute(std::string_view source,
                                  std::string_view filename = "<script>") = 0;
    
    /**
     * @brief Execute a JavaScript file
     * @param filepath Path to the JavaScript file
     * @return Result of execution or error message
     */
    virtual Result<Value> executeFile(std::string_view filepath) = 0;
    
    /**
     * @brief Register a native function in the global scope
     * @param name Name to bind the function to
     * @param callback The native callback
     */
    virtual void registerFunction(std::string_view name, NativeCallback callback) = 0;
    
    /**
     * @brief Set a global variable
     * @param name Variable name
     * @param value Value to set
     */
    virtual void setGlobal(std::string_view name, Value value) = 0;
    
    /**
     * @brief Get a global variable
     * @param name Variable name
     * @return The value or undefined if not found
     */
    virtual Value getGlobal(std::string_view name) = 0;
    
    /**
     * @brief Get the underlying Isolate
     */
    virtual Isolate* isolate() = 0;
    
    /**
     * @brief Get the current Context
     */
    virtual Context* context() = 0;
    
protected:
    ScriptEngine() = default;
};

} // namespace Zepra
