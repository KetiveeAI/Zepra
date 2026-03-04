/**
 * @file DecoratorsAPI.h
 * @brief Decorators Implementation (Stage 3 Proposal)
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <any>
#include <optional>
#include <memory>
#include <unordered_map>

namespace Zepra::Runtime {

// =============================================================================
// Decorator Context
// =============================================================================

enum class DecoratorKind {
    Class,
    Method,
    Getter,
    Setter,
    Field,
    Accessor
};

struct DecoratorContext {
    DecoratorKind kind;
    std::string name;
    bool isStatic = false;
    bool isPrivate = false;
    
    std::function<void(std::any)> addInitializer;
    std::optional<std::any> access;
    std::optional<std::any> metadata;
};

// =============================================================================
// Decorator Function Types
// =============================================================================

using ClassDecorator = std::function<std::any(std::any, DecoratorContext&)>;
using MethodDecorator = std::function<std::any(std::any, DecoratorContext&)>;
using FieldDecorator = std::function<std::any(std::any, DecoratorContext&)>;
using AccessorDecorator = std::function<std::any(std::any, DecoratorContext&)>;

// =============================================================================
// Decorator Metadata
// =============================================================================

class DecoratorMetadata {
public:
    void set(const std::string& key, std::any value) {
        data_[key] = std::move(value);
    }
    
    std::optional<std::any> get(const std::string& key) const {
        auto it = data_.find(key);
        return it != data_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    bool has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

private:
    std::unordered_map<std::string, std::any> data_;
};

// =============================================================================
// Decorator Registry
// =============================================================================

class DecoratorRegistry {
public:
    static DecoratorRegistry& instance() {
        static DecoratorRegistry registry;
        return registry;
    }
    
    void registerClassDecorator(const std::string& name, ClassDecorator decorator) {
        classDecorators_[name] = std::move(decorator);
    }
    
    void registerMethodDecorator(const std::string& name, MethodDecorator decorator) {
        methodDecorators_[name] = std::move(decorator);
    }
    
    void registerFieldDecorator(const std::string& name, FieldDecorator decorator) {
        fieldDecorators_[name] = std::move(decorator);
    }
    
    std::optional<ClassDecorator> getClassDecorator(const std::string& name) const {
        auto it = classDecorators_.find(name);
        return it != classDecorators_.end() ? std::optional(it->second) : std::nullopt;
    }
    
    std::optional<MethodDecorator> getMethodDecorator(const std::string& name) const {
        auto it = methodDecorators_.find(name);
        return it != methodDecorators_.end() ? std::optional(it->second) : std::nullopt;
    }

private:
    std::unordered_map<std::string, ClassDecorator> classDecorators_;
    std::unordered_map<std::string, MethodDecorator> methodDecorators_;
    std::unordered_map<std::string, FieldDecorator> fieldDecorators_;
};

// =============================================================================
// Decorator Application
// =============================================================================

class DecoratorApplicator {
public:
    template<typename T>
    static T applyClassDecorators(T target, const std::vector<ClassDecorator>& decorators) {
        std::any current = target;
        for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
            DecoratorContext ctx;
            ctx.kind = DecoratorKind::Class;
            auto result = (*it)(current, ctx);
            if (result.has_value()) {
                current = result;
            }
        }
        return std::any_cast<T>(current);
    }
    
    template<typename F>
    static F applyMethodDecorators(F method, const std::string& name, 
                                    const std::vector<MethodDecorator>& decorators,
                                    bool isStatic = false) {
        std::any current = method;
        for (auto it = decorators.rbegin(); it != decorators.rend(); ++it) {
            DecoratorContext ctx;
            ctx.kind = DecoratorKind::Method;
            ctx.name = name;
            ctx.isStatic = isStatic;
            auto result = (*it)(current, ctx);
            if (result.has_value()) {
                current = result;
            }
        }
        return std::any_cast<F>(current);
    }
};

// =============================================================================
// Built-in Decorators
// =============================================================================

namespace Decorators {

inline MethodDecorator logged() {
    return [](std::any method, DecoratorContext& ctx) -> std::any {
        return method;
    };
}

inline MethodDecorator bound() {
    return [](std::any method, DecoratorContext& ctx) -> std::any {
        return method;
    };
}

inline MethodDecorator deprecated(const std::string& message = "") {
    return [message](std::any method, DecoratorContext& ctx) -> std::any {
        return method;
    };
}

inline FieldDecorator readonly() {
    return [](std::any initialValue, DecoratorContext& ctx) -> std::any {
        return initialValue;
    };
}

inline ClassDecorator sealed() {
    return [](std::any constructor, DecoratorContext& ctx) -> std::any {
        return constructor;
    };
}

} // namespace Decorators

} // namespace Zepra::Runtime
