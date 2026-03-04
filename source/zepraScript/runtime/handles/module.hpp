#pragma once

/**
 * @file module.hpp
 * @brief ES6 Module System with ES2020 dynamic import
 */

#include "config.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace Zepra::Runtime {

class Object;
class Promise;

/**
 * @brief Module status enum (per ECMAScript spec)
 */
enum class ModuleStatus {
    Unlinked,
    Linking,
    Linked,
    Evaluating,
    EvaluatingAsync,
    Evaluated
};

/**
 * @brief Import entry record
 */
struct ImportEntry {
    std::string moduleRequest;  // Module specifier
    std::string importName;     // Imported binding name, '*' for namespace, 'default' for default
    std::string localName;      // Local binding name
    bool isNamespaceImport = false;
};

/**
 * @brief Export entry record
 */
struct ExportEntry {
    std::string exportName;     // Exported name, '*' for star re-export
    std::string moduleRequest;  // Re-export source (empty if local)
    std::string importName;     // Import name for re-export
    std::string localName;      // Local binding name
    bool isDefault = false;
};

/**
 * @brief import.meta object support
 */
class ImportMeta {
public:
    explicit ImportMeta(const std::string& url);
    
    std::string url() const { return url_; }
    void setResolve(std::function<std::string(const std::string&)> resolver) {
        resolver_ = resolver;
    }
    
    // resolve(specifier) - resolve relative to this module
    std::string resolve(const std::string& specifier) const {
        if (resolver_) return resolver_(specifier);
        return specifier;
    }
    
private:
    std::string url_;
    std::function<std::string(const std::string&)> resolver_;
};

/**
 * @brief ES6 Module record
 */
class Module {
public:
    explicit Module(const std::string& specifier);
    
    const std::string& specifier() const { return specifier_; }
    ModuleStatus status() const { return status_; }
    void setStatus(ModuleStatus status) { status_ = status; }
    
    void addImport(const ImportEntry& entry);
    void addExport(const ExportEntry& entry);
    
    const std::vector<ImportEntry>& imports() const { return imports_; }
    const std::vector<ExportEntry>& exports() const { return exports_; }
    
    Value getExport(const std::string& name) const;
    void setExport(const std::string& name, Value value);
    bool hasExport(const std::string& name) const;
    std::vector<std::string> exportNames() const;
    
    Object* getNamespace();
    Object* environment() const { return environment_; }
    void setEnvironment(Object* env) { environment_ = env; }
    
    // Default export
    void setDefaultExport(Value value) { defaultExport_ = value; hasDefault_ = true; }
    Value getDefaultExport() const { return defaultExport_; }
    bool hasDefaultExport() const { return hasDefault_; }
    
    // import.meta
    ImportMeta* getImportMeta();
    
private:
    std::string specifier_;
    ModuleStatus status_;
    std::vector<ImportEntry> imports_;
    std::vector<ExportEntry> exports_;
    std::unordered_map<std::string, Value> exportedBindings_;
    Object* namespace_ = nullptr;
    Object* environment_ = nullptr;
    ImportMeta* importMeta_ = nullptr;
    Value defaultExport_;
    bool hasDefault_ = false;
};

/**
 * @brief Module loader and resolver
 */
class ModuleLoader {
public:
    static ModuleLoader& instance();
    
    Module* loadModule(const std::string& specifier, const std::string& referrer = "");
    Module* getModule(const std::string& specifier) const;
    
    bool linkModule(Module* module);
    bool evaluateModule(Module* module);
    
    std::string resolveModuleSpecifier(const std::string& specifier, const std::string& referrer);
    
    // Dynamic import() - returns Promise (ES2020)
    Promise* dynamicImport(const std::string& specifier, const std::string& referrer = "");
    
    // Register built-in modules
    void registerBuiltinModule(const std::string& name, Object* exports);
    
    // Set base path
    void setBasePath(const std::string& path) { basePath_ = path; }
    
private:
    ModuleLoader();
    
    std::string readModuleSource(const std::string& path);
    
    std::unordered_map<std::string, std::unique_ptr<Module>> moduleCache_;
    std::unordered_map<std::string, Object*> builtinModules_;
    std::string basePath_;
};

/**
 * @brief Convenience functions for module operations
 */
namespace ModuleUtils {
    // Create import entry helpers
    inline ImportEntry importNamed(const std::string& module, const std::string& name) {
        return {module, name, name, false};
    }
    
    inline ImportEntry importNamedAs(const std::string& module, const std::string& name, const std::string& local) {
        return {module, name, local, false};
    }
    
    inline ImportEntry importDefault(const std::string& module, const std::string& local) {
        return {module, "default", local, false};
    }
    
    inline ImportEntry importNamespace(const std::string& module, const std::string& local) {
        return {module, "*", local, true};
    }
    
    // Create export entry helpers
    inline ExportEntry exportNamed(const std::string& name) {
        return {name, "", "", name, false};
    }
    
    inline ExportEntry exportNamedAs(const std::string& local, const std::string& exported) {
        return {exported, "", "", local, false};
    }
    
    inline ExportEntry exportDefault(const std::string& local) {
        return {"default", "", "", local, true};
    }
    
    inline ExportEntry reexportNamed(const std::string& module, const std::string& name) {
        return {name, module, name, "", false};
    }
    
    inline ExportEntry reexportAll(const std::string& module) {
        return {"*", module, "*", "", false};
    }
}

} // namespace Zepra::Runtime

