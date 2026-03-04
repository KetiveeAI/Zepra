/**
 * @file WasmModuleLink.h
 * @brief WebAssembly Module Linking and Component Model Support
 * 
 * Implements:
 * - Module imports/exports
 * - Link resolution
 * - Multi-module composition
 */

#pragma once

#include "wasm.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>

namespace Zepra::Wasm {

// =============================================================================
// Import/Export Descriptors
// =============================================================================

/**
 * @brief Types of importable/exportable items
 */
enum class ExternalKind : uint8_t {
    Function = 0,
    Table = 1,
    Memory = 2,
    Global = 3,
    Tag = 4  // For exception handling
};

/**
 * @brief Function type signature
 */
struct FuncType {
    std::vector<ValType> params;
    std::vector<ValType> results;
    
    bool operator==(const FuncType& other) const {
        return params == other.params && results == other.results;
    }
};

/**
 * @brief Table type
 */
struct TableType {
    ValType elemType;
    uint32_t minSize;
    uint32_t maxSize = UINT32_MAX;
};

/**
 * @brief Memory type
 */
struct MemoryType {
    uint32_t minPages;
    uint32_t maxPages = UINT32_MAX;
    bool shared = false;
    bool memory64 = false;
};

/**
 * @brief Global type
 */
struct GlobalType {
    ValType type;
    bool mutable_ = false;
};

/**
 * @brief Import descriptor
 */
struct ImportDesc {
    std::string module;
    std::string name;
    ExternalKind kind;
    
    std::variant<
        uint32_t,    // Function type index
        TableType,
        MemoryType,
        GlobalType
    > type;
};

/**
 * @brief Export descriptor
 */
struct ExportDesc {
    std::string name;
    ExternalKind kind;
    uint32_t index;  // Index in corresponding section
};

// =============================================================================
// Module Representation
// =============================================================================

/**
 * @brief Compiled WASM module
 */
class WasmModule {
public:
    std::string name;
    std::vector<FuncType> types;
    std::vector<ImportDesc> imports;
    std::vector<ExportDesc> exports;
    
    // Find export by name
    const ExportDesc* findExport(const std::string& name) const {
        for (const auto& exp : exports) {
            if (exp.name == name) return &exp;
        }
        return nullptr;
    }
    
    // Find import by module/name
    const ImportDesc* findImport(const std::string& module, const std::string& name) const {
        for (const auto& imp : imports) {
            if (imp.module == module && imp.name == name) return &imp;
        }
        return nullptr;
    }
    
    // Count imports by kind
    uint32_t countImports(ExternalKind kind) const {
        uint32_t count = 0;
        for (const auto& imp : imports) {
            if (imp.kind == kind) count++;
        }
        return count;
    }
};

// =============================================================================
// Link Resolution
// =============================================================================

/**
 * @brief Resolved external value
 */
struct ResolvedImport {
    ExternalKind kind;
    
    // Resolved values
    std::variant<
        void*,           // Function pointer
        void*,           // Table pointer
        void*,           // Memory pointer
        void*            // Global pointer
    > value;
    
    bool resolved = false;
};

/**
 * @brief Import provider interface
 */
class ImportProvider {
public:
    virtual ~ImportProvider() = default;
    
    virtual bool hasImport(const std::string& module, const std::string& name) const = 0;
    virtual ResolvedImport resolve(const std::string& module, const std::string& name) = 0;
};

/**
 * @brief Module-based import provider (links to another module)
 */
class ModuleImportProvider : public ImportProvider {
public:
    ModuleImportProvider(std::shared_ptr<WasmModule> module, void* instance)
        : module_(module), instance_(instance) {}
    
    bool hasImport(const std::string& module, const std::string& name) const override {
        (void)module;
        return module_->findExport(name) != nullptr;
    }
    
    ResolvedImport resolve(const std::string& module, const std::string& name) override {
        (void)module;
        ResolvedImport result;
        
        const ExportDesc* exp = module_->findExport(name);
        if (!exp) return result;
        
        result.kind = exp->kind;
        result.resolved = true;
        // Would lookup actual value from instance
        result.value = instance_;
        return result;
    }
    
private:
    std::shared_ptr<WasmModule> module_;
    void* instance_;
};

/**
 * @brief Host function import provider
 */
using HostFunction = std::function<void(void*, void*)>;

class HostImportProvider : public ImportProvider {
public:
    void addFunction(const std::string& module, const std::string& name, 
                     HostFunction func, const FuncType& type) {
        std::string key = module + "." + name;
        functions_[key] = {std::move(func), type};
    }
    
    void addMemory(const std::string& module, const std::string& name, void* memory) {
        std::string key = module + "." + name;
        memories_[key] = memory;
    }
    
    bool hasImport(const std::string& module, const std::string& name) const override {
        std::string key = module + "." + name;
        return functions_.count(key) > 0 || memories_.count(key) > 0;
    }
    
    ResolvedImport resolve(const std::string& module, const std::string& name) override {
        std::string key = module + "." + name;
        ResolvedImport result;
        
        auto funcIt = functions_.find(key);
        if (funcIt != functions_.end()) {
            result.kind = ExternalKind::Function;
            result.value = reinterpret_cast<void*>(&funcIt->second.first);
            result.resolved = true;
            return result;
        }
        
        auto memIt = memories_.find(key);
        if (memIt != memories_.end()) {
            result.kind = ExternalKind::Memory;
            result.value = memIt->second;
            result.resolved = true;
            return result;
        }
        
        return result;
    }
    
private:
    std::unordered_map<std::string, std::pair<HostFunction, FuncType>> functions_;
    std::unordered_map<std::string, void*> memories_;
};

// =============================================================================
// Link Resolver
// =============================================================================

/**
 * @brief Error during linking
 */
struct LinkError {
    std::string message;
    std::string module;
    std::string import;
};

/**
 * @brief Module linker
 */
class LinkResolver {
public:
    void addProvider(std::shared_ptr<ImportProvider> provider) {
        providers_.push_back(std::move(provider));
    }
    
    // Resolve all imports for a module
    bool resolveImports(const WasmModule& module, 
                       std::vector<ResolvedImport>& resolved,
                       std::vector<LinkError>& errors) {
        resolved.clear();
        errors.clear();
        
        for (const auto& import : module.imports) {
            bool found = false;
            
            for (const auto& provider : providers_) {
                if (provider->hasImport(import.module, import.name)) {
                    ResolvedImport res = provider->resolve(import.module, import.name);
                    if (res.resolved) {
                        resolved.push_back(std::move(res));
                        found = true;
                        break;
                    }
                }
            }
            
            if (!found) {
                errors.push_back({
                    "Unresolved import",
                    import.module,
                    import.name
                });
            }
        }
        
        return errors.empty();
    }
    
    // Type-check imports against exports
    bool validateTypes(const WasmModule& importer, const WasmModule& exporter,
                      const std::string& importName, std::string& error) {
        const ImportDesc* imp = nullptr;
        for (const auto& i : importer.imports) {
            if (i.name == importName) {
                imp = &i;
                break;
            }
        }
        
        const ExportDesc* exp = exporter.findExport(importName);
        if (!imp || !exp) {
            error = "Import/export not found";
            return false;
        }
        
        if (imp->kind != exp->kind) {
            error = "Kind mismatch";
            return false;
        }
        
        // Would do detailed type checking here
        return true;
    }
    
private:
    std::vector<std::shared_ptr<ImportProvider>> providers_;
};

// =============================================================================
// Linked Module
// =============================================================================

/**
 * @brief Post-linking module representation
 */
class LinkedModule {
public:
    std::shared_ptr<WasmModule> module;
    std::vector<ResolvedImport> resolvedImports;
    
    // Get resolved function by import index
    void* getFunction(uint32_t importIndex) const {
        if (importIndex < resolvedImports.size() &&
            resolvedImports[importIndex].kind == ExternalKind::Function) {
            return std::get<void*>(resolvedImports[importIndex].value);
        }
        return nullptr;
    }
    
    // Get resolved memory by import index
    void* getMemory(uint32_t importIndex) const {
        if (importIndex < resolvedImports.size() &&
            resolvedImports[importIndex].kind == ExternalKind::Memory) {
            return std::get<void*>(resolvedImports[importIndex].value);
        }
        return nullptr;
    }
};

} // namespace Zepra::Wasm
