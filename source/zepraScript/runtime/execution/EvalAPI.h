/**
 * @file EvalAPI.h
 * @brief Eval and Script Execution
 */

#pragma once

#include <string>
#include <functional>
#include <optional>
#include <any>
#include <memory>

namespace Zepra::Runtime {

// =============================================================================
// Script Source
// =============================================================================

struct ScriptSource {
    std::string code;
    std::string filename;
    int lineOffset = 0;
    int columnOffset = 0;
    bool isModule = false;
};

// =============================================================================
// Eval Options
// =============================================================================

struct EvalOptions {
    bool strict = false;
    bool direct = true;
    std::string filename = "<eval>";
};

// =============================================================================
// Script Result
// =============================================================================

struct ScriptResult {
    std::any value;
    bool success = true;
    std::string error;
    
    static ScriptResult success(std::any val) {
        return {std::move(val), true, ""};
    }
    
    static ScriptResult failure(const std::string& err) {
        return {{}, false, err};
    }
};

// =============================================================================
// Eval Context
// =============================================================================

class EvalContext {
public:
    using GlobalGetter = std::function<std::optional<std::any>(const std::string&)>;
    using GlobalSetter = std::function<bool(const std::string&, std::any)>;
    
    void setGlobalGetter(GlobalGetter getter) { globalGetter_ = std::move(getter); }
    void setGlobalSetter(GlobalSetter setter) { globalSetter_ = std::move(setter); }
    
    std::optional<std::any> getGlobal(const std::string& name) const {
        return globalGetter_ ? globalGetter_(name) : std::nullopt;
    }
    
    bool setGlobal(const std::string& name, std::any value) {
        return globalSetter_ ? globalSetter_(name, std::move(value)) : false;
    }

private:
    GlobalGetter globalGetter_;
    GlobalSetter globalSetter_;
};

// =============================================================================
// Eval Function
// =============================================================================

class Eval {
public:
    using EvalHandler = std::function<ScriptResult(const std::string&, const EvalOptions&)>;
    
    explicit Eval(EvalHandler handler = nullptr) : handler_(std::move(handler)) {}
    
    ScriptResult operator()(const std::string& code, EvalOptions opts = {}) const {
        if (!handler_) {
            return ScriptResult::failure("eval is not supported");
        }
        return handler_(code, opts);
    }
    
    static bool isDirectEval(bool calledFromEval) {
        return calledFromEval;
    }

private:
    EvalHandler handler_;
};

// =============================================================================
// Function Constructor
// =============================================================================

class FunctionConstructor {
public:
    using CreateHandler = std::function<std::any(const std::vector<std::string>&, const std::string&)>;
    
    explicit FunctionConstructor(CreateHandler handler = nullptr) : handler_(std::move(handler)) {}
    
    std::any operator()(const std::vector<std::string>& paramNames, const std::string& body) const {
        if (!handler_) {
            return {};
        }
        return handler_(paramNames, body);
    }
    
    template<typename... Args>
    std::any create(Args&&... args) const {
        std::vector<std::string> params;
        std::string body;
        collectArgs(params, body, std::forward<Args>(args)...);
        return (*this)(params, body);
    }

private:
    template<typename T, typename... Rest>
    void collectArgs(std::vector<std::string>& params, std::string& body, T&& first, Rest&&... rest) const {
        if constexpr (sizeof...(rest) == 0) {
            body = std::forward<T>(first);
        } else {
            params.push_back(std::forward<T>(first));
            collectArgs(params, body, std::forward<Rest>(rest)...);
        }
    }
    
    void collectArgs(std::vector<std::string>&, std::string&) const {}
    
    CreateHandler handler_;
};

} // namespace Zepra::Runtime
