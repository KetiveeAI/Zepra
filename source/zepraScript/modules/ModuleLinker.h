/**
 * @file ModuleLinker.h
 * @brief Module linking and dependency resolution
 * 
 * Implements:
 * - Topological sort of module graph
 * - Circular dependency detection
 * - Export/import binding resolution
 * - Live binding support
 * 
 * Based on ES Module linking specification
 */

#pragma once

#include "ModuleLoader.h"
#include <vector>
#include <unordered_set>
#include <stack>

namespace Zepra::Modules {

// =============================================================================
// Resolved Binding
// =============================================================================

struct ResolvedBinding {
    ModuleRecord* module;       // The module containing the binding
    std::string bindingName;    // The local binding name in that module
    bool isAmbiguous = false;   // Ambiguous export (multiple sources)
    bool isNamespace = false;   // Module namespace object
    
    bool isValid() const { return module != nullptr && !isAmbiguous; }
};

// =============================================================================
// Link Error
// =============================================================================

struct LinkError {
    enum class Type {
        CircularDependency,
        UnresolvedImport,
        AmbiguousExport,
        DuplicateExport,
        SyntaxError
    };
    
    Type type;
    std::string message;
    std::string moduleSpecifier;
    std::string importName;
    
    std::string format() const;
};

// =============================================================================
// Module Graph
// =============================================================================

class ModuleGraph {
public:
    void addModule(ModuleRecord* module);
    void addEdge(ModuleRecord* from, ModuleRecord* to);
    
    /**
     * @brief Get all modules in topological order
     */
    std::vector<ModuleRecord*> topologicalSort() const;
    
    /**
     * @brief Check for circular dependencies
     */
    bool hasCircularDependency() const;
    
    /**
     * @brief Get circular dependency path (if exists)
     */
    std::vector<ModuleRecord*> getCircularPath() const;
    
    /**
     * @brief Get all modules
     */
    const std::vector<ModuleRecord*>& modules() const { return modules_; }
    
    /**
     * @brief Get dependents of a module
     */
    std::vector<ModuleRecord*> getDependents(ModuleRecord* module) const;
    
private:
    std::vector<ModuleRecord*> modules_;
    std::unordered_map<ModuleRecord*, std::vector<ModuleRecord*>> edges_;
    
    bool hasCycleFrom(ModuleRecord* start, 
                      std::unordered_set<ModuleRecord*>& visited,
                      std::unordered_set<ModuleRecord*>& stack,
                      std::vector<ModuleRecord*>& path) const;
};

// =============================================================================
// Module Linker
// =============================================================================

class ModuleLinker {
public:
    ModuleLinker() = default;
    
    // =========================================================================
    // Linking
    // =========================================================================
    
    /**
     * @brief Link a module and all its dependencies
     */
    bool link(ModuleRecord* module);
    
    /**
     * @brief Link multiple modules
     */
    bool linkAll(const std::vector<ModuleRecord*>& modules);
    
    // =========================================================================
    // Export Resolution
    // =========================================================================
    
    /**
     * @brief Resolve an export from a module
     */
    ResolvedBinding resolveExport(ModuleRecord* module, 
                                   const std::string& exportName,
                                   std::unordered_set<ModuleRecord*>& resolveSet);
    
    /**
     * @brief Get all exports from a module
     */
    std::vector<std::string> getExportedNames(ModuleRecord* module,
                                              std::unordered_set<ModuleRecord*>& exportStars);
    
    // =========================================================================
    // Import Resolution
    // =========================================================================
    
    /**
     * @brief Resolve an import binding
     */
    ResolvedBinding resolveImport(ModuleRecord* module, const ImportEntry& import);
    
    /**
     * @brief Resolve all imports for a module
     */
    bool resolveAllImports(ModuleRecord* module);
    
    // =========================================================================
    // Errors
    // =========================================================================
    
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<LinkError>& errors() const { return errors_; }
    void clearErrors() { errors_.clear(); }
    
    // =========================================================================
    // Graph Access
    // =========================================================================
    
    const ModuleGraph& graph() const { return graph_; }
    
private:
    ModuleGraph graph_;
    std::vector<LinkError> errors_;
    
    bool innerModuleLinking(ModuleRecord* module,
                            std::stack<ModuleRecord*>& stack,
                            size_t& index);
    
    void addError(LinkError::Type type, 
                  const std::string& message,
                  const std::string& module,
                  const std::string& name = "");
};

// =============================================================================
// Live Binding
// =============================================================================

/**
 * @brief Represents a live binding between modules
 */
class LiveBinding {
public:
    LiveBinding(ModuleRecord* sourceModule, const std::string& sourceName)
        : sourceModule_(sourceModule), sourceName_(sourceName) {}
    
    ModuleRecord* sourceModule() const { return sourceModule_; }
    const std::string& sourceName() const { return sourceName_; }
    
    // Binding can be updated when source changes
    void invalidate() { valid_ = false; }
    bool isValid() const { return valid_; }
    
private:
    ModuleRecord* sourceModule_;
    std::string sourceName_;
    bool valid_ = true;
};

} // namespace Zepra::Modules
