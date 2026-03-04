/**
 * @file class_fields.hpp
 * @brief ES2022 Class Fields support
 * 
 * Implements:
 * - Public instance fields
 * - Private instance fields (#field)
 * - Public static fields
 * - Private static fields
 * - Private methods
 * - Static initialization blocks
 */

#pragma once

#include "config.hpp"
#include "value.hpp"
#include "object.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Zepra::Runtime {

class Function;

/**
 * @brief Private field name (Symbol-like, unique per class)
 */
class PrivateFieldName {
public:
    explicit PrivateFieldName(const std::string& name)
        : name_(name)
        , id_(nextId_++) {}
    
    const std::string& name() const { return name_; }
    uint64_t id() const { return id_; }
    
    bool operator==(const PrivateFieldName& other) const {
        return id_ == other.id_;
    }
    
private:
    std::string name_;
    uint64_t id_;
    static uint64_t nextId_;
};

/**
 * @brief Hash function for PrivateFieldName
 */
struct PrivateFieldNameHash {
    size_t operator()(const PrivateFieldName& name) const {
        return std::hash<uint64_t>{}(name.id());
    }
};

/**
 * @brief Class field definition
 */
struct ClassField {
    std::string name;
    bool isPrivate = false;
    bool isStatic = false;
    bool isMethod = false;
    bool isAccessor = false;  // getter/setter
    Value initializer;        // For fields
    Function* method = nullptr;  // For methods
    Function* getter = nullptr;
    Function* setter = nullptr;
};

/**
 * @brief Static initialization block
 */
struct StaticInitBlock {
    Function* code = nullptr;
};

/**
 * @brief Class definition with ES2022 features
 */
class ClassDefinition {
public:
    explicit ClassDefinition(const std::string& name);
    
    const std::string& name() const { return name_; }
    
    // Inheritance
    void setExtends(Object* superClass) { superClass_ = superClass; }
    Object* superClass() const { return superClass_; }
    
    // Constructor
    void setConstructor(Function* ctor) { constructor_ = ctor; }
    Function* constructor() const { return constructor_; }
    
    // Fields
    void addPublicField(const std::string& name, Value initializer);
    void addPrivateField(const std::string& name, Value initializer);
    void addStaticField(const std::string& name, Value initializer, bool isPrivate = false);
    
    // Methods
    void addMethod(const std::string& name, Function* method, bool isStatic = false);
    void addPrivateMethod(const std::string& name, Function* method, bool isStatic = false);
    
    // Accessors
    void addGetter(const std::string& name, Function* getter, bool isPrivate = false, bool isStatic = false);
    void addSetter(const std::string& name, Function* setter, bool isPrivate = false, bool isStatic = false);
    
    // Static blocks
    void addStaticBlock(Function* code);
    
    // Build the class
    Function* build();
    
    // Get private field name (for compiler)
    const PrivateFieldName& getPrivateFieldName(const std::string& name);
    
    // All fields
    const std::vector<ClassField>& fields() const { return fields_; }
    const std::vector<StaticInitBlock>& staticBlocks() const { return staticBlocks_; }
    
private:
    std::string name_;
    Object* superClass_ = nullptr;
    Function* constructor_ = nullptr;
    std::vector<ClassField> fields_;
    std::vector<StaticInitBlock> staticBlocks_;
    std::unordered_map<std::string, PrivateFieldName> privateNames_;
};

/**
 * @brief Private field storage for object instances
 */
class PrivateFields {
public:
    void set(const PrivateFieldName& name, Value value);
    Value get(const PrivateFieldName& name) const;
    bool has(const PrivateFieldName& name) const;
    
private:
    std::unordered_map<PrivateFieldName, Value, PrivateFieldNameHash> fields_;
};

/**
 * @brief Brand check for private field access
 */
class ClassBrand {
public:
    explicit ClassBrand(uint64_t id) : id_(id) {}
    
    uint64_t id() const { return id_; }
    
    bool operator==(const ClassBrand& other) const {
        return id_ == other.id_;
    }
    
    static ClassBrand create() {
        return ClassBrand(nextId_++);
    }
    
private:
    uint64_t id_;
    static uint64_t nextId_;
};

/**
 * @brief Object with private fields support
 */
class ClassInstance : public Object {
public:
    explicit ClassInstance(const ClassBrand& brand);
    
    // Private field access
    void setPrivate(const PrivateFieldName& name, Value value);
    Value getPrivate(const PrivateFieldName& name) const;
    bool hasPrivate(const PrivateFieldName& name) const;
    
    // Brand check (for #field in obj)
    bool hasBrand(const ClassBrand& brand) const { return brand_ == brand; }
    
private:
    ClassBrand brand_;
    PrivateFields privateFields_;
};

/**
 * @brief Helper for creating class objects
 */
namespace ClassUtils {
    // Create a new class from definition
    Function* createClass(ClassDefinition& def);
    
    // Initialize instance fields
    void initializeInstanceFields(Object* instance, ClassDefinition& def);
    
    // Initialize static fields and run static blocks
    void initializeStatics(Function* classConstructor, ClassDefinition& def);
    
    // Check if object has private field (for #field in obj)
    bool hasPrivateField(Object* obj, const PrivateFieldName& name);
}

} // namespace Zepra::Runtime
