/**
 * @file ModuleLoader.h
 * @brief ES Module loading and resolution
 * 
 * Implements:
 * - Module specifier resolution
 * - Dynamic import()
 * - Module caching
 * - Circular dependency handling
 * 
 * Based on ES Module specification
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>
#include <future>

namespace Zepra::Modules {

// Forward declarations
class Module;
class ModuleRecord;

// =============================================================================
// Module Specifier
// =============================================================================

struct ModuleSpecifier {
    std::string raw;            // Original specifier from import
    std::string resolved;       // Resolved URL/path
    bool isBare;                // Bare specifier (no ./ or /)
    bool isRelative;            // Starts with ./ or ../
    bool isAbsolute;            // Starts with / or URL
    
    static ModuleSpecifier parse(const std::string& specifier);
};

// =============================================================================
// Module Status
// =============================================================================

enum class ModuleStatus : uint8_t {
    Unlinked,       // Module parsed but not linked
    Linking,        // Currently being linked
    Linked,         // Linked but not evaluated
    Evaluating,     // Currently being evaluated
    Evaluated,      // Successfully evaluated
    Error           // Error during linking/evaluation
};

// =============================================================================
// Import/Export Entry
// =============================================================================

struct ImportEntry {
    std::string moduleRequest;      // Source module specifier
    std::string importName;         // Name in source module
    std::string localName;          // Local binding name
};

struct ExportEntry {
    std::string exportName;         // Exported name
    std::string localName;          // Local binding (for local exports)
    std::string moduleRequest;      // Re-export source (optional)
    std::string importName;         // Re-export import name (optional)
    
    bool isReExport() const { return !moduleRequest.empty(); }
    bool isNamespaceExport() const { return exportName == "*"; }
};

// =============================================================================
// Module Record
// =============================================================================

class ModuleRecord {
public:
    ModuleRecord(const std::string& specifier, const std::string& source);
    
    // Identity
    const std::string& specifier() const { return specifier_; }
    const std::string& sourceText() const { return source_; }
    
    // Status
    ModuleStatus status() const { return status_; }
    void setStatus(ModuleStatus status) { status_ = status; }
    
    // Imports/Exports
    const std::vector<ImportEntry>& imports() const { return imports_; }
    const std::vector<ExportEntry>& exports() const { return exports_; }
    
    void addImport(const ImportEntry& entry) { imports_.push_back(entry); }
    void addExport(const ExportEntry& entry) { exports_.push_back(entry); }
    
    // Requested modules (dependencies)
    const std::vector<std::string>& requestedModules() const { return requestedModules_; }
    void addRequestedModule(const std::string& specifier) {
        requestedModules_.push_back(specifier);
    }
    
    // Linked module references
    void setResolvedModule(const std::string& specifier, ModuleRecord* module) {
        resolvedModules_[specifier] = module;
    }
    ModuleRecord* getResolvedModule(const std::string& specifier) const {
        auto it = resolvedModules_.find(specifier);
        return it != resolvedModules_.end() ? it->second : nullptr;
    }
    
    // Error
    void setError(const std::string& error) { 
        status_ = ModuleStatus::Error;
        error_ = error;
    }
    const std::string& error() const { return error_; }
    bool hasError() const { return status_ == ModuleStatus::Error; }
    
private:
    std::string specifier_;
    std::string source_;
    ModuleStatus status_ = ModuleStatus::Unlinked;
    std::string error_;
    
    std::vector<ImportEntry> imports_;
    std::vector<ExportEntry> exports_;
    std::vector<std::string> requestedModules_;
    std::unordered_map<std::string, ModuleRecord*> resolvedModules_;
};

// =============================================================================
// Module Loader Hooks
// =============================================================================

struct ModuleLoaderHooks {
    // Resolve specifier relative to referrer
    using ResolveHook = std::function<std::string(
        const std::string& specifier,
        const std::string& referrer)>;
    
    // Fetch module source
    using FetchHook = std::function<std::future<std::string>(
        const std::string& url)>;
    
    // Parse module source to record
    using ParseHook = std::function<std::unique_ptr<ModuleRecord>(
        const std::string& specifier,
        const std::string& source)>;
    
    ResolveHook resolve;
    FetchHook fetch;
    ParseHook parse;
};

// =============================================================================
// Module Loader
// =============================================================================

class ModuleLoader {
public:
    explicit ModuleLoader(ModuleLoaderHooks hooks);
    
    // =========================================================================
    // Loading
    // =========================================================================
    
    /**
     * @brief Load module and all dependencies
     */
    std::future<ModuleRecord*> loadModule(const std::string& specifier,
                                           const std::string& referrer = "");
    
    /**
     * @brief Synchronous load (for Workers)
     */
    ModuleRecord* loadModuleSync(const std::string& specifier,
                                  const std::string& referrer = "");
    
    /**
     * @brief Dynamic import()
     */
    std::future<ModuleRecord*> dynamicImport(const std::string& specifier,
                                              const std::string& referrer);
    
    // =========================================================================
    // Resolution
    // =========================================================================
    
    /**
     * @brief Resolve specifier to URL
     */
    std::string resolve(const std::string& specifier, const std::string& referrer);
    
    // =========================================================================
    // Cache
    // =========================================================================
    
    /**
     * @brief Get cached module by URL
     */
    ModuleRecord* getCached(const std::string& url) const;
    
    /**
     * @brief Clear module cache
     */
    void clearCache();
    
    // =========================================================================
    // Import Maps
    // =========================================================================
    
    struct ImportMap {
        std::unordered_map<std::string, std::string> imports;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> scopes;
    };
    
    void setImportMap(ImportMap map) { importMap_ = std::move(map); }
    
    /**
     * @brief Apply import map to specifier
     */
    std::optional<std::string> resolveWithImportMap(const std::string& specifier,
                                                      const std::string& referrer) const;
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct Stats {
        size_t modulesLoaded = 0;
        size_t modulesCached = 0;
        size_t fetchCount = 0;
        size_t cacheHits = 0;
    };
    
    Stats getStats() const { return stats_; }
    
private:
    ModuleLoaderHooks hooks_;
    std::unordered_map<std::string, std::unique_ptr<ModuleRecord>> cache_;
    ImportMap importMap_;
    Stats stats_;
    
    ModuleRecord* loadModuleInternal(const std::string& url);
    void loadDependencies(ModuleRecord* record);
};

// =============================================================================
// Default Hooks
// =============================================================================

namespace DefaultHooks {

/**
 * @brief File system resolver
 */
std::string fileSystemResolve(const std::string& specifier, const std::string& referrer);

/**
 * @brief Node.js style resolver
 */
std::string nodeResolve(const std::string& specifier, const std::string& referrer);

/**
 * @brief Browser URL resolver
 */
std::string browserResolve(const std::string& specifier, const std::string& referrer);

} // namespace DefaultHooks

} // namespace Zepra::Modules
