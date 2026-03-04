/**
 * @file ExecutionContext.h
 * @brief Execution context for script evaluation
 * 
 * Implements:
 * - Global/function execution contexts
 * - Lexical environment chain
 * - Realm abstraction
 * - Context stack
 * 
 * Based on ECMAScript execution context specification
 */

#pragma once

#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace Zepra::VM {

// Forward declarations
class VirtualMachine;
class Function;
class Script;

// =============================================================================
// Environment Record Types
// =============================================================================

enum class EnvironmentType : uint8_t {
    Declarative,    // Function/block scope
    Object,         // With statement, global
    Global,         // Global environment
    Module          // ES6 module environment
};

// =============================================================================
// Binding
// =============================================================================

struct Binding {
    Runtime::Value value;
    bool isMutable = true;
    bool isInitialized = false;
    bool canDelete = true;
    bool isStrict = false;
};

// =============================================================================
// Environment Record
// =============================================================================

class EnvironmentRecord {
public:
    explicit EnvironmentRecord(EnvironmentType type) : type_(type) {}
    virtual ~EnvironmentRecord() = default;
    
    EnvironmentType type() const { return type_; }
    
    // Binding operations
    virtual bool hasBinding(const std::string& name) const;
    virtual void createMutableBinding(const std::string& name, bool canDelete = true);
    virtual void createImmutableBinding(const std::string& name, bool strict = false);
    virtual void initializeBinding(const std::string& name, Runtime::Value value);
    virtual Runtime::Value getBindingValue(const std::string& name, bool strict = false);
    virtual void setMutableBinding(const std::string& name, Runtime::Value value, bool strict = false);
    virtual bool deleteBinding(const std::string& name);
    virtual bool hasThisBinding() const { return false; }
    virtual Runtime::Value getThisBinding() { return Runtime::Value::undefined(); }
    
    // Outer environment
    EnvironmentRecord* outer() const { return outer_; }
    void setOuter(EnvironmentRecord* env) { outer_ = env; }
    
protected:
    EnvironmentType type_;
    EnvironmentRecord* outer_ = nullptr;
    std::unordered_map<std::string, Binding> bindings_;
};

// =============================================================================
// Global Environment
// =============================================================================

class GlobalEnvironment : public EnvironmentRecord {
public:
    explicit GlobalEnvironment(Runtime::Object* globalObject);
    
    bool hasBinding(const std::string& name) const override;
    Runtime::Value getBindingValue(const std::string& name, bool strict) override;
    void setMutableBinding(const std::string& name, Runtime::Value value, bool strict) override;
    
    bool hasVarDeclaration(const std::string& name) const;
    bool hasLexicalDeclaration(const std::string& name) const;
    bool canDeclareGlobalVar(const std::string& name) const;
    bool canDeclareGlobalFunction(const std::string& name) const;
    
    void createGlobalVarBinding(const std::string& name, bool canDelete);
    void createGlobalFunctionBinding(const std::string& name, Runtime::Value value, bool canDelete);
    
    Runtime::Object* globalObject() const { return globalObject_; }
    
private:
    Runtime::Object* globalObject_;
    std::unordered_map<std::string, Binding> varNames_;
};

// =============================================================================
// Function Environment
// =============================================================================

class FunctionEnvironment : public EnvironmentRecord {
public:
    FunctionEnvironment();
    
    bool hasThisBinding() const override { return thisBindingStatus_ != ThisBindingStatus::Uninitialized; }
    Runtime::Value getThisBinding() override;
    void bindThisValue(Runtime::Value value);
    
    // Super binding
    bool hasSuperBinding() const { return homeObject_ != nullptr; }
    Runtime::Object* getSuperBase();
    void setHomeObject(Runtime::Object* obj) { homeObject_ = obj; }
    
    // new.target
    Runtime::Value getNewTarget() const { return newTarget_; }
    void setNewTarget(Runtime::Value target) { newTarget_ = target; }
    
private:
    enum class ThisBindingStatus { Lexical, Initialized, Uninitialized };
    
    ThisBindingStatus thisBindingStatus_ = ThisBindingStatus::Uninitialized;
    Runtime::Value thisValue_;
    Runtime::Object* homeObject_ = nullptr;
    Runtime::Value newTarget_;
};

// =============================================================================
// Execution Context
// =============================================================================

class ExecutionContext {
public:
    enum class Type { Global, Function, Eval, Module };
    
    ExecutionContext(Type type, EnvironmentRecord* lexicalEnv, EnvironmentRecord* variableEnv);
    
    Type type() const { return type_; }
    
    // Environments
    EnvironmentRecord* lexicalEnvironment() const { return lexicalEnv_; }
    EnvironmentRecord* variableEnvironment() const { return variableEnv_; }
    void setLexicalEnvironment(EnvironmentRecord* env) { lexicalEnv_ = env; }
    void setVariableEnvironment(EnvironmentRecord* env) { variableEnv_ = env; }
    
    // Function context
    Function* function() const { return function_; }
    void setFunction(Function* func) { function_ = func; }
    
    // Script association
    Script* script() const { return script_; }
    void setScript(Script* script) { script_ = script; }
    
    // Realm
    // Realm* realm() const { return realm_; }
    
    // Strict mode
    bool isStrict() const { return strict_; }
    void setStrict(bool strict) { strict_ = strict; }
    
private:
    Type type_;
    EnvironmentRecord* lexicalEnv_;
    EnvironmentRecord* variableEnv_;
    Function* function_ = nullptr;
    Script* script_ = nullptr;
    bool strict_ = false;
};

// =============================================================================
// Execution Context Stack
// =============================================================================

class ExecutionContextStack {
public:
    void push(std::unique_ptr<ExecutionContext> ctx) {
        stack_.push_back(std::move(ctx));
    }
    
    std::unique_ptr<ExecutionContext> pop() {
        if (stack_.empty()) return nullptr;
        auto ctx = std::move(stack_.back());
        stack_.pop_back();
        return ctx;
    }
    
    ExecutionContext* running() const {
        return stack_.empty() ? nullptr : stack_.back().get();
    }
    
    size_t depth() const { return stack_.size(); }
    bool empty() const { return stack_.empty(); }
    
private:
    std::vector<std::unique_ptr<ExecutionContext>> stack_;
};

// =============================================================================
// Context Helpers
// =============================================================================

/**
 * @brief RAII helper for pushing/popping execution context
 */
class ExecutionContextScope {
public:
    ExecutionContextScope(ExecutionContextStack& stack, 
                          std::unique_ptr<ExecutionContext> ctx)
        : stack_(stack) {
        stack_.push(std::move(ctx));
    }
    
    ~ExecutionContextScope() {
        stack_.pop();
    }
    
private:
    ExecutionContextStack& stack_;
};

/**
 * @brief Resolve identifier in environment chain
 */
inline EnvironmentRecord* resolveBinding(EnvironmentRecord* env, 
                                         const std::string& name) {
    while (env) {
        if (env->hasBinding(name)) return env;
        env = env->outer();
    }
    return nullptr;
}

} // namespace Zepra::VM
