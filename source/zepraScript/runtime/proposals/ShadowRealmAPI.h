/**
 * @file ShadowRealmAPI.h
 * @brief ShadowRealm Implementation
 */

#pragma once

#include <string>
#include <functional>
#include <any>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace Zepra::Runtime {

// =============================================================================
// Realm Interface
// =============================================================================

class Realm {
public:
    virtual ~Realm() = default;
    virtual std::any evaluate(const std::string& code) = 0;
    virtual std::any importValue(const std::string& specifier, const std::string& bindingName) = 0;
};

// =============================================================================
// ShadowRealm
// =============================================================================

class ShadowRealm : public Realm {
public:
    using EvalHandler = std::function<std::any(const std::string&)>;
    using ImportHandler = std::function<std::any(const std::string&, const std::string&)>;
    
    ShadowRealm() = default;
    
    void setEvalHandler(EvalHandler handler) { evalHandler_ = std::move(handler); }
    void setImportHandler(ImportHandler handler) { importHandler_ = std::move(handler); }
    
    std::any evaluate(const std::string& sourceText) override {
        if (!evalHandler_) {
            throw std::runtime_error("ShadowRealm: evaluate not supported");
        }
        
        try {
            return wrapForExport(evalHandler_(sourceText));
        } catch (const std::exception& e) {
            throw TypeError(std::string("ShadowRealm evaluation error: ") + e.what());
        }
    }
    
    std::any importValue(const std::string& specifier, const std::string& bindingName) override {
        if (!importHandler_) {
            throw std::runtime_error("ShadowRealm: importValue not supported");
        }
        
        try {
            return wrapForExport(importHandler_(specifier, bindingName));
        } catch (const std::exception& e) {
            throw TypeError(std::string("ShadowRealm import error: ") + e.what());
        }
    }
    
    // Callable wrapper creation
    template<typename R, typename... Args>
    std::function<R(Args...)> wrapCallable(std::function<R(Args...)> fn) {
        return [this, fn](Args... args) -> R {
            return fn(std::forward<Args>(args)...);
        };
    }

private:
    std::any wrapForExport(std::any value) {
        if (isPrimitive(value)) return value;
        if (isCallable(value)) return wrapCallableForExport(value);
        throw TypeError("ShadowRealm: only primitives and callables can cross realm boundary");
    }
    
    bool isPrimitive(const std::any& value) {
        return value.type() == typeid(int) ||
               value.type() == typeid(double) ||
               value.type() == typeid(bool) ||
               value.type() == typeid(std::string) ||
               !value.has_value();
    }
    
    bool isCallable(const std::any& value) {
        return value.type() == typeid(std::function<std::any(std::any)>);
    }
    
    std::any wrapCallableForExport(const std::any& callable) {
        auto fn = std::any_cast<std::function<std::any(std::any)>>(callable);
        return std::function<std::any(std::any)>([this, fn](std::any arg) {
            return wrapForExport(fn(arg));
        });
    }
    
    class TypeError : public std::runtime_error {
    public:
        explicit TypeError(const std::string& msg) : std::runtime_error(msg) {}
    };
    
    EvalHandler evalHandler_;
    ImportHandler importHandler_;
};

// =============================================================================
// Realm Registry
// =============================================================================

class RealmRegistry {
public:
    static RealmRegistry& instance() {
        static RealmRegistry registry;
        return registry;
    }
    
    std::shared_ptr<ShadowRealm> create() {
        auto realm = std::make_shared<ShadowRealm>();
        realms_.push_back(realm);
        return realm;
    }
    
    void remove(const std::shared_ptr<ShadowRealm>& realm) {
        realms_.erase(
            std::remove(realms_.begin(), realms_.end(), realm),
            realms_.end()
        );
    }

private:
    std::vector<std::shared_ptr<ShadowRealm>> realms_;
};

// =============================================================================
// Factory
// =============================================================================

inline std::shared_ptr<ShadowRealm> createShadowRealm() {
    return RealmRegistry::instance().create();
}

} // namespace Zepra::Runtime
