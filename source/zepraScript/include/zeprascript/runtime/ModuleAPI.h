/**
 * @file ModuleAPI.h
 * @brief ES Module System Implementation
 * 
 * ECMAScript Modules based on:
 * - JSC JSModuleRecord.h
 * - SpiderMonkey ModuleObject.h
 * - ECMA-262 15.2 Modules
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// Forward Declarations
// =============================================================================

class ModuleRecord;
class ModuleEnvironment;
class ModuleNamespace;

// =============================================================================
// Module Status (per ECMA-262)
// =============================================================================

enum class ModuleStatus {
    New,              // Initial state
    Unlinked,         // Parsed but not linked
    Linking,          // Being linked
    Linked,           // Linked successfully
    Evaluating,       // Being evaluated
    EvaluatingAsync,  // Async evaluation in progress
    Evaluated         // Evaluation complete
};

// =============================================================================
// Import Entry
// =============================================================================

struct ImportEntry {
    std::string moduleRequest;    // Module specifier
    std::string importName;       // Exported name (or "*" for namespace)
    std::string localName;        // Local binding name
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;
    
    bool isNamespaceImport() const { return importName == "*"; }
};

// =============================================================================
// Export Entry
// =============================================================================

struct ExportEntry {
    std::optional<std::string> exportName;      // Export name (nullopt for re-export all)
    std::optional<std::string> moduleRequest;   // Source module (nullopt for local)
    std::optional<std::string> importName;      // Import name from source
    std::optional<std::string> localName;       // Local binding name
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;
    
    bool isLocalExport() const { return !moduleRequest.has_value(); }
    bool isIndirectExport() const { return moduleRequest.has_value() && importName.has_value(); }
    bool isStarExport() const { return moduleRequest.has_value() && !importName.has_value(); }
};

// =============================================================================
// Resolved Binding
// =============================================================================

struct ResolvedBinding {
    std::shared_ptr<ModuleRecord> module;
    std::string bindingName;
    bool isAmbiguous = false;
    
    static ResolvedBinding ambiguous() {
        ResolvedBinding rb;
        rb.isAmbiguous = true;
        return rb;
    }
    
    bool isValid() const { return module != nullptr && !isAmbiguous; }
};

// =============================================================================
// Module Environment
// =============================================================================

class ModuleEnvironment {
public:
    using Value = std::variant<std::monostate, bool, double, std::string, std::shared_ptr<void>>;
    
    ModuleEnvironment() = default;
    explicit ModuleEnvironment(std::shared_ptr<ModuleEnvironment> outer)
        : outer_(outer) {}
    
    bool hasBinding(const std::string& name) const {
        return bindings_.find(name) != bindings_.end();
    }
    
    void createBinding(const std::string& name, bool isMutable = true, bool isStrict = true) {
        bindings_[name] = BindingInfo{Value{}, isMutable, isStrict, false};
    }
    
    void createImmutableBinding(const std::string& name, bool isStrict = true) {
        bindings_[name] = BindingInfo{Value{}, false, isStrict, false};
    }
    
    void initializeBinding(const std::string& name, Value value) {
        auto it = bindings_.find(name);
        if (it != bindings_.end()) {
            it->second.value = std::move(value);
            it->second.initialized = true;
        }
    }
    
    std::optional<Value> getBindingValue(const std::string& name) const {
        auto it = bindings_.find(name);
        if (it != bindings_.end() && it->second.initialized) {
            return it->second.value;
        }
        if (outer_) {
            return outer_->getBindingValue(name);
        }
        return std::nullopt;
    }
    
    bool setMutableBinding(const std::string& name, Value value) {
        auto it = bindings_.find(name);
        if (it != bindings_.end()) {
            if (!it->second.isMutable) {
                return false;  // TypeError: assignment to const
            }
            it->second.value = std::move(value);
            return true;
        }
        if (outer_) {
            return outer_->setMutableBinding(name, std::move(value));
        }
        return false;
    }
    
    void createImportBinding(const std::string& localName,
                             std::shared_ptr<ModuleRecord> targetModule,
                             const std::string& targetName) {
        importBindings_[localName] = {targetModule, targetName};
    }
    
private:
    struct BindingInfo {
        Value value;
        bool isMutable = true;
        bool isStrict = true;
        bool initialized = false;
    };
    
    struct ImportBinding {
        std::shared_ptr<ModuleRecord> module;
        std::string name;
    };
    
    std::unordered_map<std::string, BindingInfo> bindings_;
    std::unordered_map<std::string, ImportBinding> importBindings_;
    std::shared_ptr<ModuleEnvironment> outer_;
};

// =============================================================================
// Module Namespace Object
// =============================================================================

class ModuleNamespace {
public:
    explicit ModuleNamespace(std::shared_ptr<ModuleRecord> module)
        : module_(module) {}
    
    void addExport(const std::string& name) {
        exports_.push_back(name);
    }
    
    const std::vector<std::string>& exports() const { return exports_; }
    std::shared_ptr<ModuleRecord> module() const { return module_; }
    
    bool hasExport(const std::string& name) const {
        return std::find(exports_.begin(), exports_.end(), name) != exports_.end();
    }
    
private:
    std::shared_ptr<ModuleRecord> module_;
    std::vector<std::string> exports_;
};

// =============================================================================
// Abstract Module Record
// =============================================================================

class ModuleRecord : public std::enable_shared_from_this<ModuleRecord> {
public:
    virtual ~ModuleRecord() = default;
    
    // Module specifier
    const std::string& specifier() const { return specifier_; }
    void setSpecifier(std::string spec) { specifier_ = std::move(spec); }
    
    // Status
    ModuleStatus status() const { return status_; }
    void setStatus(ModuleStatus s) { status_ = s; }
    
    // Environment
    std::shared_ptr<ModuleEnvironment> environment() const { return environment_; }
    void setEnvironment(std::shared_ptr<ModuleEnvironment> env) { environment_ = std::move(env); }
    
    // Namespace
    std::shared_ptr<ModuleNamespace> namespace_() const { return namespace__; }
    void setNamespace(std::shared_ptr<ModuleNamespace> ns) { namespace__ = std::move(ns); }
    
    // Evaluation error
    bool hasEvaluationError() const { return evaluationError_.has_value(); }
    const std::string& evaluationError() const { return *evaluationError_; }
    void setEvaluationError(std::string error) { evaluationError_ = std::move(error); }
    
    // Abstract methods
    virtual std::vector<std::string> getExportedNames(std::vector<std::shared_ptr<ModuleRecord>>& exportStarSet) = 0;
    virtual ResolvedBinding resolveExport(const std::string& name, std::vector<std::pair<std::shared_ptr<ModuleRecord>, std::string>>& resolveSet) = 0;
    virtual bool link() = 0;
    virtual bool evaluate() = 0;
    
protected:
    std::string specifier_;
    ModuleStatus status_ = ModuleStatus::New;
    std::shared_ptr<ModuleEnvironment> environment_;
    std::shared_ptr<ModuleNamespace> namespace__;
    std::optional<std::string> evaluationError_;
};

// =============================================================================
// Source Text Module Record
// =============================================================================

class SourceTextModuleRecord : public ModuleRecord {
public:
    SourceTextModuleRecord() = default;
    
    // Import/Export entries
    const std::vector<ImportEntry>& importEntries() const { return importEntries_; }
    const std::vector<ExportEntry>& localExportEntries() const { return localExportEntries_; }
    const std::vector<ExportEntry>& indirectExportEntries() const { return indirectExportEntries_; }
    const std::vector<ExportEntry>& starExportEntries() const { return starExportEntries_; }
    
    void addImportEntry(ImportEntry entry) { importEntries_.push_back(std::move(entry)); }
    void addLocalExportEntry(ExportEntry entry) { localExportEntries_.push_back(std::move(entry)); }
    void addIndirectExportEntry(ExportEntry entry) { indirectExportEntries_.push_back(std::move(entry)); }
    void addStarExportEntry(ExportEntry entry) { starExportEntries_.push_back(std::move(entry)); }
    
    // Requested modules
    const std::vector<std::string>& requestedModules() const { return requestedModules_; }
    void addRequestedModule(std::string spec) { requestedModules_.push_back(std::move(spec)); }
    
    // Loaded modules map
    void setLoadedModule(const std::string& specifier, std::shared_ptr<ModuleRecord> module) {
        loadedModules_[specifier] = std::move(module);
    }
    
    std::shared_ptr<ModuleRecord> getLoadedModule(const std::string& specifier) const {
        auto it = loadedModules_.find(specifier);
        return it != loadedModules_.end() ? it->second : nullptr;
    }
    
    // Top-level await
    bool hasTopLevelAwait() const { return hasTopLevelAwait_; }
    void setHasTopLevelAwait(bool v) { hasTopLevelAwait_ = v; }
    
    // Async evaluation
    bool isAsyncEvaluation() const { return asyncEvaluation_; }
    void setAsyncEvaluation(bool v) { asyncEvaluation_ = v; }
    
    // DFS indices for linking
    uint32_t dfsIndex() const { return dfsIndex_; }
    void setDfsIndex(uint32_t i) { dfsIndex_ = i; }
    
    uint32_t dfsAncestorIndex() const { return dfsAncestorIndex_; }
    void setDfsAncestorIndex(uint32_t i) { dfsAncestorIndex_ = i; }
    
    // Implementation of abstract methods
    std::vector<std::string> getExportedNames(std::vector<std::shared_ptr<ModuleRecord>>& exportStarSet) override {
        if (std::find(exportStarSet.begin(), exportStarSet.end(), shared_from_this()) != exportStarSet.end()) {
            return {};
        }
        exportStarSet.push_back(std::static_pointer_cast<ModuleRecord>(shared_from_this()));
        
        std::vector<std::string> names;
        
        for (const auto& e : localExportEntries_) {
            if (e.exportName) {
                names.push_back(*e.exportName);
            }
        }
        
        for (const auto& e : indirectExportEntries_) {
            if (e.exportName) {
                names.push_back(*e.exportName);
            }
        }
        
        for (const auto& e : starExportEntries_) {
            if (auto requestedModule = getLoadedModule(*e.moduleRequest)) {
                auto starNames = requestedModule->getExportedNames(exportStarSet);
                for (const auto& n : starNames) {
                    if (n != "default" && std::find(names.begin(), names.end(), n) == names.end()) {
                        names.push_back(n);
                    }
                }
            }
        }
        
        return names;
    }
    
    ResolvedBinding resolveExport(const std::string& name, 
                                   std::vector<std::pair<std::shared_ptr<ModuleRecord>, std::string>>& resolveSet) override {
        for (const auto& [mod, n] : resolveSet) {
            if (mod.get() == this && n == name) {
                return {};  // Circular
            }
        }
        resolveSet.emplace_back(std::static_pointer_cast<ModuleRecord>(shared_from_this()), name);
        
        for (const auto& e : localExportEntries_) {
            if (e.exportName && *e.exportName == name) {
                return ResolvedBinding{std::static_pointer_cast<ModuleRecord>(shared_from_this()), *e.localName};
            }
        }
        
        for (const auto& e : indirectExportEntries_) {
            if (e.exportName && *e.exportName == name) {
                if (auto importedModule = getLoadedModule(*e.moduleRequest)) {
                    if (*e.importName == "*") {
                        return ResolvedBinding{importedModule, "*namespace*"};
                    }
                    return importedModule->resolveExport(*e.importName, resolveSet);
                }
            }
        }
        
        if (name == "default") {
            return {};
        }
        
        ResolvedBinding starResolution;
        for (const auto& e : starExportEntries_) {
            if (auto importedModule = getLoadedModule(*e.moduleRequest)) {
                auto resolution = importedModule->resolveExport(name, resolveSet);
                if (resolution.isAmbiguous) {
                    return resolution;
                }
                if (resolution.isValid()) {
                    if (starResolution.isValid()) {
                        if (resolution.module != starResolution.module ||
                            resolution.bindingName != starResolution.bindingName) {
                            return ResolvedBinding::ambiguous();
                        }
                    } else {
                        starResolution = resolution;
                    }
                }
            }
        }
        
        return starResolution;
    }
    
    bool link() override {
        if (status_ != ModuleStatus::Unlinked) {
            return status_ == ModuleStatus::Linked;
        }
        
        status_ = ModuleStatus::Linking;
        environment_ = std::make_shared<ModuleEnvironment>();
        
        for (const auto& entry : importEntries_) {
            auto importedModule = getLoadedModule(entry.moduleRequest);
            if (!importedModule) {
                return false;
            }
            
            if (entry.isNamespaceImport()) {
                auto ns = importedModule->namespace_();
                if (!ns) {
                    ns = std::make_shared<ModuleNamespace>(importedModule);
                    importedModule->setNamespace(ns);
                }
                environment_->createImmutableBinding(entry.localName);
                environment_->initializeBinding(entry.localName, ns);
            } else {
                std::vector<std::pair<std::shared_ptr<ModuleRecord>, std::string>> resolveSet;
                auto resolution = importedModule->resolveExport(entry.importName, resolveSet);
                if (!resolution.isValid()) {
                    return false;
                }
                environment_->createImportBinding(entry.localName, resolution.module, resolution.bindingName);
            }
        }
        
        status_ = ModuleStatus::Linked;
        return true;
    }
    
    bool evaluate() override {
        if (status_ == ModuleStatus::Evaluated) {
            return !hasEvaluationError();
        }
        
        if (status_ != ModuleStatus::Linked) {
            return false;
        }
        
        status_ = hasTopLevelAwait_ ? ModuleStatus::EvaluatingAsync : ModuleStatus::Evaluating;
        
        for (const auto& specifier : requestedModules_) {
            if (auto dep = getLoadedModule(specifier)) {
                if (!dep->evaluate()) {
                    status_ = ModuleStatus::Evaluated;
                    evaluationError_ = "Dependency evaluation failed: " + specifier;
                    return false;
                }
            }
        }
        
        status_ = ModuleStatus::Evaluated;
        return true;
    }
    
private:
    std::vector<ImportEntry> importEntries_;
    std::vector<ExportEntry> localExportEntries_;
    std::vector<ExportEntry> indirectExportEntries_;
    std::vector<ExportEntry> starExportEntries_;
    std::vector<std::string> requestedModules_;
    std::unordered_map<std::string, std::shared_ptr<ModuleRecord>> loadedModules_;
    
    bool hasTopLevelAwait_ = false;
    bool asyncEvaluation_ = false;
    uint32_t dfsIndex_ = 0;
    uint32_t dfsAncestorIndex_ = 0;
};

// =============================================================================
// Module Loader
// =============================================================================

class ModuleLoader {
public:
    using ResolveCallback = std::function<std::string(const std::string& specifier, const std::string& referrer)>;
    using FetchCallback = std::function<std::string(const std::string& url)>;
    
    ModuleLoader() = default;
    
    void setResolveHook(ResolveCallback cb) { resolveHook_ = std::move(cb); }
    void setFetchHook(FetchCallback cb) { fetchHook_ = std::move(cb); }
    
    std::shared_ptr<ModuleRecord> getModule(const std::string& specifier) const {
        auto it = registry_.find(specifier);
        return it != registry_.end() ? it->second : nullptr;
    }
    
    void registerModule(const std::string& specifier, std::shared_ptr<ModuleRecord> module) {
        registry_[specifier] = std::move(module);
    }
    
    std::string resolve(const std::string& specifier, const std::string& referrer) {
        if (resolveHook_) {
            return resolveHook_(specifier, referrer);
        }
        return specifier;
    }
    
    std::optional<std::string> fetch(const std::string& url) {
        if (fetchHook_) {
            return fetchHook_(url);
        }
        return std::nullopt;
    }
    
    bool linkModule(std::shared_ptr<ModuleRecord> module) {
        return module->link();
    }
    
    bool evaluateModule(std::shared_ptr<ModuleRecord> module) {
        return module->evaluate();
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<ModuleRecord>> registry_;
    ResolveCallback resolveHook_;
    FetchCallback fetchHook_;
};

} // namespace Zepra::Runtime
