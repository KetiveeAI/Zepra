/**
 * @file FunctionAPI.h
 * @brief Function Implementation
 */

#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <any>
#include <optional>

namespace Zepra::Runtime {

class Function {
public:
    using Args = std::vector<std::any>;
    using Callable = std::function<std::any(std::any, Args)>;
    
    Function() = default;
    explicit Function(Callable fn, std::string name = "")
        : callable_(std::move(fn)), name_(std::move(name)) {}
    
    // Properties
    const std::string& name() const { return name_; }
    size_t length() const { return expectedArgs_; }
    
    // Call methods
    std::any call(std::any thisArg, const Args& args = {}) const {
        if (!callable_) return {};
        return callable_(thisArg, args);
    }
    
    std::any apply(std::any thisArg, const Args& args = {}) const {
        return call(thisArg, args);
    }
    
    template<typename... Ts>
    std::any operator()(std::any thisArg, Ts&&... args) const {
        return call(thisArg, Args{std::any(std::forward<Ts>(args))...});
    }
    
    // Bind
    Function bind(std::any boundThis, const Args& boundArgs = {}) const {
        auto originalFn = callable_;
        auto newFn = [originalFn, boundThis, boundArgs](std::any, Args args) -> std::any {
            Args combinedArgs = boundArgs;
            combinedArgs.insert(combinedArgs.end(), args.begin(), args.end());
            return originalFn(boundThis, combinedArgs);
        };
        
        Function bound(newFn, "bound " + name_);
        bound.boundThis_ = boundThis;
        bound.boundArgs_ = boundArgs;
        return bound;
    }
    
    // toString
    std::string toString() const {
        if (isNative_) {
            return "function " + name_ + "() { [native code] }";
        }
        return "function " + name_ + "() { [code] }";
    }
    
    // Check if callable
    explicit operator bool() const { return static_cast<bool>(callable_); }
    
    // Prototype chain
    std::shared_ptr<Function> getPrototype() const { return prototype_; }
    void setPrototype(std::shared_ptr<Function> proto) { prototype_ = std::move(proto); }

private:
    Callable callable_;
    std::string name_;
    size_t expectedArgs_ = 0;
    bool isNative_ = true;
    std::any boundThis_;
    Args boundArgs_;
    std::shared_ptr<Function> prototype_;
};

// Arrow function (lexically bound this)
class ArrowFunction : public Function {
public:
    ArrowFunction(Callable fn, std::any lexicalThis)
        : Function(std::move(fn)), lexicalThis_(std::move(lexicalThis)) {}
    
    std::any call(std::any, const Args& args = {}) const {
        return Function::call(lexicalThis_, args);
    }

private:
    std::any lexicalThis_;
};

// Async function wrapper
class AsyncFunction : public Function {
public:
    using Function::Function;
    bool isAsync() const { return true; }
};

// Generator function wrapper
class GeneratorFunction : public Function {
public:
    using Function::Function;
    bool isGenerator() const { return true; }
};

} // namespace Zepra::Runtime
