#pragma once

/**
 * @file environment.hpp
 * @brief Lexical environment (scope chain)
 */

#include "../config.hpp"
#include "value.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

namespace Zepra::Runtime {

/**
 * @brief Variable binding entry
 */
struct Binding {
    Value value = Value::undefined();
    bool mutable_ = true;      // false for const
    bool initialized = false;  // for TDZ
};

/**
 * @brief Lexical environment (scope)
 * 
 * Implements ECMAScript Lexical Environment and Environment Record.
 */
class Environment {
public:
    /**
     * @brief Create a new environment
     * @param outer The enclosing environment (parent scope)
     */
    explicit Environment(Environment* outer = nullptr);
    
    /**
     * @brief Create a new variable binding
     * @param name Variable name
     * @param mutable_ true for var/let, false for const
     */
    void createBinding(const std::string& name, bool mutable_ = true);
    
    /**
     * @brief Initialize a binding
     * @param name Variable name
     * @param value Initial value
     */
    void initializeBinding(const std::string& name, Value value);
    
    /**
     * @brief Set a binding value
     * @param name Variable name
     * @param value New value
     * @return true if successful, false if binding doesn't exist or is immutable
     */
    bool setBinding(const std::string& name, Value value);
    
    /**
     * @brief Get a binding value
     * @param name Variable name
     * @return The value, or undefined if not found
     */
    Value getBinding(const std::string& name) const;
    
    /**
     * @brief Check if a binding exists in this environment (not parent)
     */
    bool hasOwnBinding(const std::string& name) const;
    
    /**
     * @brief Check if a binding exists (including parent scopes)
     */
    bool hasBinding(const std::string& name) const;
    
    /**
     * @brief Delete a binding (for var declarations)
     */
    bool deleteBinding(const std::string& name);
    
    /**
     * @brief Get the outer (parent) environment
     */
    Environment* outer() const { return outer_; }
    
    /**
     * @brief Find the environment containing a binding
     */
    Environment* resolve(const std::string& name);
    const Environment* resolve(const std::string& name) const;
    
    /**
     * @brief Create child environment
     */
    std::unique_ptr<Environment> createChild();
    
    /**
     * @brief Get all binding names in this environment
     */
    std::vector<std::string> bindingNames() const;
    
private:
    Environment* outer_;
    std::unordered_map<std::string, Binding> bindings_;
};

/**
 * @brief Global environment
 * 
 * Special environment that includes global object properties.
 */
class GlobalEnvironment : public Environment {
public:
    explicit GlobalEnvironment(Object* globalObject);
    
    Object* globalObject() const { return globalObject_; }
    
    // Override to also check global object
    Value getBinding(const std::string& name) const;
    bool hasBinding(const std::string& name) const;
    bool setBinding(const std::string& name, Value value);
    
private:
    Object* globalObject_;
};

} // namespace Zepra::Runtime
