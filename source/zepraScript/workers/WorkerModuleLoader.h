/**
 * @file WorkerModuleLoader.h
 * @brief Module loading for Web Workers
 * 
 * Implements:
 * - Worker module scripts
 * - SharedWorker module support
 * - Worklet modules
    * - Import.meta in workers
 */

#pragma once

#include "../modules/ModuleLoader.h"
#include "../modules/ModuleExecutor.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Zepra::Workers {

// =============================================================================
// Worker Type
// =============================================================================

enum class WorkerType : uint8_t {
    DedicatedWorker,
    SharedWorker,
    ServiceWorker,
    Worklet
};

// =============================================================================
// Worker Script Type
// =============================================================================

enum class WorkerScriptType : uint8_t {
    Classic,    // Classic script semantics
    Module      // ES Module semantics
};

// =============================================================================
// Import Meta
// =============================================================================

struct ImportMeta {
    std::string url;
    std::string resolve(const std::string& specifier) const;
    
    // Custom properties (extensible)
    std::unordered_map<std::string, Runtime::Value> properties;
};

// =============================================================================
// Worker Module Context
// =============================================================================

class WorkerModuleContext {
public:
    WorkerModuleContext(WorkerType type, const std::string& scriptUrl);
    
    WorkerType workerType() const { return type_; }
    const std::string& scriptUrl() const { return scriptUrl_; }
    
    // Module loader for this worker
    Modules::ModuleLoader& loader() { return loader_; }
    Modules::ModuleExecutor& executor() { return executor_; }
    
    // Import.meta for current module
    ImportMeta& importMeta() { return importMeta_; }
    const ImportMeta& importMeta() const { return importMeta_; }
    
    // GC integration
    void registerWithGC(GC::GCController* gc);
    
private:
    WorkerType type_;
    std::string scriptUrl_;
    Modules::ModuleLoader loader_;
    Modules::ModuleLinker linker_;
    Modules::ModuleExecutor executor_;
    ImportMeta importMeta_;
};

// =============================================================================
// Worker Module Loader
// =============================================================================

class WorkerModuleLoader {
public:
    explicit WorkerModuleLoader(WorkerType type);
    
    // =========================================================================
    // Module Loading
    // =========================================================================
    
    /**
     * @brief Load worker entry module
     */
    std::future<bool> loadEntryModule(const std::string& url, WorkerScriptType type);
    
    /**
     * @brief Dynamic import() in worker
     */
    std::future<Runtime::Value> dynamicImport(const std::string& specifier);
    
    /**
     * @brief Import scripts (classic worker)
     */
    bool importScripts(const std::vector<std::string>& urls);
    
    // =========================================================================
    // Execution
    // =========================================================================
    
    /**
     * @brief Execute loaded modules
     */
    bool execute();
    
    /**
     * @brief Check if ready
     */
    bool isReady() const { return ready_.load(); }
    
    // =========================================================================
    // Context Access
    // =========================================================================
    
    WorkerModuleContext& context() { return context_; }
    const WorkerModuleContext& context() const { return context_; }
    
private:
    WorkerModuleContext context_;
    std::atomic<bool> ready_{false};
    Modules::ModuleRecord* entryModule_ = nullptr;
};

// =============================================================================
// Shared Worker Registry
// =============================================================================

class SharedWorkerRegistry {
public:
    static SharedWorkerRegistry& instance();
    
    /**
     * @brief Get or create shared worker
     */
    WorkerModuleLoader* getOrCreate(const std::string& url, const std::string& name);
    
    /**
     * @brief Check if shared worker exists
     */
    bool exists(const std::string& url, const std::string& name) const;
    
    /**
     * @brief Remove shared worker
     */
    void remove(const std::string& url, const std::string& name);
    
private:
    SharedWorkerRegistry() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<WorkerModuleLoader>> workers_;
    
    std::string makeKey(const std::string& url, const std::string& name) const;
};

} // namespace Zepra::Workers
