/**
 * @file environment.cpp
 * @brief Lexical environment implementation
 */

#include "runtime/execution/environment.hpp"
#include "runtime/objects/object.hpp"

namespace Zepra::Runtime {

Environment::Environment(Environment* outer)
    : outer_(outer)
{
}

void Environment::createBinding(const std::string& name, bool mutable_) {
    Binding binding;
    binding.mutable_ = mutable_;
    binding.initialized = false;
    bindings_[name] = binding;
}

void Environment::initializeBinding(const std::string& name, Value value) {
    auto it = bindings_.find(name);
    if (it != bindings_.end()) {
        it->second.value = value;
        it->second.initialized = true;
    } else {
        // Create and initialize in one step
        Binding binding;
        binding.value = value;
        binding.mutable_ = true;
        binding.initialized = true;
        bindings_[name] = binding;
    }
}

bool Environment::setBinding(const std::string& name, Value value) {
    auto env = resolve(name);
    if (!env) {
        return false;
    }
    
    auto it = env->bindings_.find(name);
    if (it == env->bindings_.end()) {
        return false;
    }
    
    // Check if mutable
    if (!it->second.mutable_) {
        return false; // Cannot assign to const
    }
    
    it->second.value = value;
    return true;
}

Value Environment::getBinding(const std::string& name) const {
    const Environment* env = resolve(name);
    if (!env) {
        return Value::undefined();
    }
    
    auto it = env->bindings_.find(name);
    if (it == env->bindings_.end()) {
        return Value::undefined();
    }
    
    // Check TDZ
    if (!it->second.initialized) {
        // In a real engine, we'd throw ReferenceError
        return Value::undefined();
    }
    
    return it->second.value;
}

bool Environment::hasOwnBinding(const std::string& name) const {
    return bindings_.find(name) != bindings_.end();
}

bool Environment::hasBinding(const std::string& name) const {
    return resolve(name) != nullptr;
}

bool Environment::deleteBinding(const std::string& name) {
    auto it = bindings_.find(name);
    if (it != bindings_.end()) {
        bindings_.erase(it);
        return true;
    }
    return false;
}

Environment* Environment::resolve(const std::string& name) {
    if (hasOwnBinding(name)) {
        return this;
    }
    if (outer_) {
        return outer_->resolve(name);
    }
    return nullptr;
}

const Environment* Environment::resolve(const std::string& name) const {
    if (hasOwnBinding(name)) {
        return this;
    }
    if (outer_) {
        return outer_->resolve(name);
    }
    return nullptr;
}

std::unique_ptr<Environment> Environment::createChild() {
    return std::make_unique<Environment>(this);
}

std::vector<std::string> Environment::bindingNames() const {
    std::vector<std::string> names;
    names.reserve(bindings_.size());
    for (const auto& [name, _] : bindings_) {
        names.push_back(name);
    }
    return names;
}

// =============================================================================
// GlobalEnvironment
// =============================================================================

GlobalEnvironment::GlobalEnvironment(Object* globalObject)
    : Environment(nullptr)
    , globalObject_(globalObject)
{
}

Value GlobalEnvironment::getBinding(const std::string& name) const {
    // First check lexical bindings (let/const)
    if (hasOwnBinding(name)) {
        return Environment::getBinding(name);
    }
    
    // Then check global object properties (var, function declarations)
    if (globalObject_ && globalObject_->has(name)) {
        return globalObject_->get(name);
    }
    
    return Value::undefined();
}

bool GlobalEnvironment::hasBinding(const std::string& name) const {
    if (hasOwnBinding(name)) {
        return true;
    }
    return globalObject_ && globalObject_->has(name);
}

bool GlobalEnvironment::setBinding(const std::string& name, Value value) {
    // Check own lexical bindings first
    if (hasOwnBinding(name)) {
        return Environment::setBinding(name, value);
    }
    
    // Set on global object
    if (globalObject_) {
        return globalObject_->set(name, value);
    }
    
    return false;
}

} // namespace Zepra::Runtime
