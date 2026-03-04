/**
 * @file class_fields.cpp
 * @brief ES2022 Class Fields implementation
 */

#include "runtime/objects/class_fields.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Runtime {

// Static counters for unique IDs
uint64_t PrivateFieldName::nextId_ = 1;
uint64_t ClassBrand::nextId_ = 1;

// =============================================================================
// ClassDefinition
// =============================================================================

ClassDefinition::ClassDefinition(const std::string& name)
    : name_(name) {}

void ClassDefinition::addPublicField(const std::string& name, Value initializer) {
    fields_.push_back({name, false, false, false, false, initializer, nullptr, nullptr, nullptr});
}

void ClassDefinition::addPrivateField(const std::string& name, Value initializer) {
    // Ensure we have a PrivateFieldName for this field
    if (privateNames_.find(name) == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
    }
    fields_.push_back({name, true, false, false, false, initializer, nullptr, nullptr, nullptr});
}

void ClassDefinition::addStaticField(const std::string& name, Value initializer, bool isPrivate) {
    if (isPrivate && privateNames_.find(name) == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
    }
    fields_.push_back({name, isPrivate, true, false, false, initializer, nullptr, nullptr, nullptr});
}

void ClassDefinition::addMethod(const std::string& name, Function* method, bool isStatic) {
    fields_.push_back({name, false, isStatic, true, false, Value::undefined(), method, nullptr, nullptr});
}

void ClassDefinition::addPrivateMethod(const std::string& name, Function* method, bool isStatic) {
    if (privateNames_.find(name) == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
    }
    fields_.push_back({name, true, isStatic, true, false, Value::undefined(), method, nullptr, nullptr});
}

void ClassDefinition::addGetter(const std::string& name, Function* getter, bool isPrivate, bool isStatic) {
    if (isPrivate && privateNames_.find(name) == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
    }
    // Check if accessor already exists
    for (auto& field : fields_) {
        if (field.name == name && field.isAccessor && field.isPrivate == isPrivate && field.isStatic == isStatic) {
            field.getter = getter;
            return;
        }
    }
    fields_.push_back({name, isPrivate, isStatic, false, true, Value::undefined(), nullptr, getter, nullptr});
}

void ClassDefinition::addSetter(const std::string& name, Function* setter, bool isPrivate, bool isStatic) {
    if (isPrivate && privateNames_.find(name) == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
    }
    // Check if accessor already exists
    for (auto& field : fields_) {
        if (field.name == name && field.isAccessor && field.isPrivate == isPrivate && field.isStatic == isStatic) {
            field.setter = setter;
            return;
        }
    }
    fields_.push_back({name, isPrivate, isStatic, false, true, Value::undefined(), nullptr, nullptr, setter});
}

void ClassDefinition::addStaticBlock(Function* code) {
    staticBlocks_.push_back({code});
}

const PrivateFieldName& ClassDefinition::getPrivateFieldName(const std::string& name) {
    auto it = privateNames_.find(name);
    if (it == privateNames_.end()) {
        privateNames_.emplace(name, PrivateFieldName(name));
        return privateNames_.at(name);
    }
    return it->second;
}

Function* ClassDefinition::build() {
    // Create the constructor function
    Function* ctor = constructor_;
    if (!ctor) {
        // Create default constructor
        ctor = new Function(name_, [](const FunctionCallInfo&) -> Value {
            return Value::undefined();
        }, 0);
    }
    
    // Set up prototype
    Object* prototype = new Object();
    ctor->set("prototype", Value::object(prototype));
    prototype->set("constructor", Value::object(ctor));
    
    // If extending, set up prototype chain
    if (superClass_) {
        // We want Sub.prototype.__proto__ = Base.prototype
        // superClass_ is Base (constructor), so we need its "prototype" property
        Value baseProtoVal = superClass_->get("prototype");
        if (baseProtoVal.isObject()) {
            prototype->setPrototype(baseProtoVal.asObject());
        }
    }
    
    // Add instance methods to prototype
    for (const auto& field : fields_) {
        if (!field.isStatic && field.isMethod && !field.isPrivate) {
            prototype->set(field.name, Value::object(field.method));
        }
        if (!field.isStatic && field.isAccessor && !field.isPrivate) {
            // Define property with getter/setter
            // Simplified - actual implementation would use defineProperty
            if (field.getter) {
                prototype->set("get " + field.name, Value::object(field.getter));
            }
            if (field.setter) {
                prototype->set("set " + field.name, Value::object(field.setter));
            }
        }
    }
    
    // Add static methods to constructor
    for (const auto& field : fields_) {
        if (field.isStatic && field.isMethod && !field.isPrivate) {
            ctor->set(field.name, Value::object(field.method));
        }
        if (field.isStatic && !field.isMethod && !field.isAccessor && !field.isPrivate) {
            // Static field
            ctor->set(field.name, field.initializer);
        }
    }
    
    // Run static initialization blocks
    for (const auto& block : staticBlocks_) {
        if (block.code) {
            // Execute static block with nullptr context
            std::vector<Value> emptyArgs;
            block.code->call(nullptr, Value::undefined(), emptyArgs);
        }
    }
    
    return ctor;
}

// =============================================================================
// PrivateFields
// =============================================================================

void PrivateFields::set(const PrivateFieldName& name, Value value) {
    fields_[name] = value;
}

Value PrivateFields::get(const PrivateFieldName& name) const {
    auto it = fields_.find(name);
    if (it != fields_.end()) {
        return it->second;
    }
    return Value::undefined();
}

bool PrivateFields::has(const PrivateFieldName& name) const {
    return fields_.find(name) != fields_.end();
}

// =============================================================================
// ClassInstance
// =============================================================================

ClassInstance::ClassInstance(const ClassBrand& brand)
    : Object(ObjectType::Ordinary)
    , brand_(brand) {}

void ClassInstance::setPrivate(const PrivateFieldName& name, Value value) {
    privateFields_.set(name, value);
}

Value ClassInstance::getPrivate(const PrivateFieldName& name) const {
    return privateFields_.get(name);
}

bool ClassInstance::hasPrivate(const PrivateFieldName& name) const {
    return privateFields_.has(name);
}

// =============================================================================
// ClassUtils
// =============================================================================

namespace ClassUtils {

Function* createClass(ClassDefinition& def) {
    return def.build();
}

void initializeInstanceFields(Object* instance, ClassDefinition& def) {
    for (const auto& field : def.fields()) {
        if (!field.isStatic && !field.isMethod && !field.isAccessor) {
            if (field.isPrivate) {
                // Private field - need ClassInstance
                if (auto* classInstance = dynamic_cast<ClassInstance*>(instance)) {
                    classInstance->setPrivate(
                        def.getPrivateFieldName(field.name),
                        field.initializer
                    );
                }
            } else {
                // Public field
                instance->set(field.name, field.initializer);
            }
        }
    }
}

void initializeStatics(Function* classConstructor, ClassDefinition& def) {
    for (const auto& field : def.fields()) {
        if (field.isStatic && !field.isMethod && !field.isAccessor && !field.isPrivate) {
            classConstructor->set(field.name, field.initializer);
        }
    }
    
    // Run static blocks
    for (const auto& block : def.staticBlocks()) {
        if (block.code) {
            std::vector<Value> emptyArgs;
            block.code->call(nullptr, Value::undefined(), emptyArgs);
        }
    }
}

bool hasPrivateField(Object* obj, const PrivateFieldName& name) {
    if (auto* classInstance = dynamic_cast<ClassInstance*>(obj)) {
        return classInstance->hasPrivate(name);
    }
    return false;
}

} // namespace ClassUtils

} // namespace Zepra::Runtime
