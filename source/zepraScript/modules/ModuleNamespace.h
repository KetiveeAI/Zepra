/**
 * @file ModuleNamespace.h
 * @brief Module namespace object implementation
 * 
 * Implements:
 * - Module namespace exotic object
 * - Export iteration
 * - Property access
 * - Symbol.toStringTag
 * 
 * Based on ES Module namespace object spec
 */

#pragma once

#include "ModuleLoader.h"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Zepra::Modules {

// =============================================================================
// Module Namespace
// =============================================================================

/**
 * @brief ES Module Namespace exotic object
 * 
 * Provides:
 * - Read-only access to module exports
 * - Iterator over exported names
 * - Symbol.toStringTag = "Module"
 */
class ModuleNamespace {
public:
    explicit ModuleNamespace(ModuleRecord* module);
    
    // =========================================================================
    // Identity
    // =========================================================================
    
    ModuleRecord* module() const { return module_; }
    
    // =========================================================================
    // [[GetPrototypeOf]]
    // =========================================================================
    
    static Runtime::Value getPrototypeOf() { return Runtime::Value::null(); }
    
    // =========================================================================
    // [[SetPrototypeOf]]
    // =========================================================================
    
    static bool setPrototypeOf(Runtime::Value proto) {
        return proto.isNull(); // Only null allowed
    }
    
    // =========================================================================
    // [[IsExtensible]]
    // =========================================================================
    
    static bool isExtensible() { return false; }
    
    // =========================================================================
    // [[PreventExtensions]]
    // =========================================================================
    
    static bool preventExtensions() { return true; }
    
    // =========================================================================
    // [[GetOwnProperty]]
    // =========================================================================
    
    struct PropertyDescriptor {
        Runtime::Value value;
        bool writable = true;
        bool enumerable = true;
        bool configurable = false;
        bool found = false;
    };
    
    PropertyDescriptor getOwnProperty(const std::string& key) const;
    
    // =========================================================================
    // [[DefineOwnProperty]]
    // =========================================================================
    
    bool defineOwnProperty(const std::string& key, const PropertyDescriptor& desc) {
        return false; // Cannot define properties
    }
    
    // =========================================================================
    // [[HasProperty]]
    // =========================================================================
    
    bool has(const std::string& key) const;
    
    // =========================================================================
    // [[Get]]
    // =========================================================================
    
    Runtime::Value get(const std::string& key) const;
    
    // =========================================================================
    // [[Set]]
    // =========================================================================
    
    bool set(const std::string& key, Runtime::Value value) {
        return false; // Read-only
    }
    
    // =========================================================================
    // [[Delete]]
    // =========================================================================
    
    bool deleteProperty(const std::string& key) {
        return false; // Cannot delete
    }
    
    // =========================================================================
    // [[OwnPropertyKeys]]
    // =========================================================================
    
    std::vector<std::string> ownPropertyKeys() const;
    
    // =========================================================================
    // Export Access
    // =========================================================================
    
    /**
     * @brief Get all exported names
     */
    const std::vector<std::string>& exportedNames() const { return exports_; }
    
    /**
     * @brief Get exported value by name
     */
    Runtime::Value getExport(const std::string& name) const;
    
    /**
     * @brief Check if name is exported
     */
    bool hasExport(const std::string& name) const;
    
    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize namespace from module exports
     */
    void initialize();
    
    /**
     * @brief Called after module evaluation to finalize bindings
     */
    void finalize();
    
private:
    ModuleRecord* module_;
    std::vector<std::string> exports_;
    std::unordered_map<std::string, Runtime::Value> bindings_;
    bool initialized_ = false;
    bool finalized_ = false;
};

// =============================================================================
// Module Environment Record
// =============================================================================

/**
 * @brief Environment record for module scope
 */
class ModuleEnvironmentRecord {
public:
    explicit ModuleEnvironmentRecord(ModuleRecord* module);
    
    // =========================================================================
    // Binding Operations
    // =========================================================================
    
    /**
     * @brief Create immutable binding (for imports)
     */
    void createImmutableBinding(const std::string& name, bool strict);
    
    /**
     * @brief Create mutable binding (for local declarations)
     */
    void createMutableBinding(const std::string& name, bool deletable);
    
    /**
     * @brief Initialize binding with value
     */
    void initializeBinding(const std::string& name, Runtime::Value value);
    
    /**
     * @brief Set mutable binding
     */
    bool setMutableBinding(const std::string& name, Runtime::Value value, bool strict);
    
    /**
     * @brief Get binding value
     */
    Runtime::Value getBindingValue(const std::string& name, bool strict) const;
    
    /**
     * @brief Delete binding
     */
    bool deleteBinding(const std::string& name);
    
    /**
     * @brief Check if binding exists
     */
    bool hasBinding(const std::string& name) const;
    
    /**
     * @brief Check if binding is initialized
     */
    bool isInitialized(const std::string& name) const;
    
    // =========================================================================
    // Import Bindings
    // =========================================================================
    
    /**
     * @brief Create import binding (indirect binding to another module)
     */
    void createImportBinding(const std::string& localName,
                             ModuleRecord* targetModule,
                             const std::string& targetName);
    
    /**
     * @brief Get this module's namespace object
     */
    ModuleNamespace* getModuleNamespace();
    
private:
    struct Binding {
        Runtime::Value value;
        bool mutable_ = true;
        bool initialized = false;
        bool deletable = false;
        
        // For import bindings
        bool isImport = false;
        ModuleRecord* importModule = nullptr;
        std::string importName;
    };
    
    ModuleRecord* module_;
    std::unordered_map<std::string, Binding> bindings_;
    std::unique_ptr<ModuleNamespace> namespace_;
};

} // namespace Zepra::Modules
