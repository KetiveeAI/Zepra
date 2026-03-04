/**
 * @file WorkerModuleLoader.cpp
 * @brief Worker module loading implementation
 */

#include "workers/WorkerModuleLoader.h"
#include <sstream>
#include <stdexcept>

namespace Zepra::Workers {

// =============================================================================
// ImportMeta
// =============================================================================

std::string ImportMeta::resolve(const std::string& specifier) const {
    // Relative resolution against module URL
    if (specifier.empty()) return url;
    
    if (specifier[0] == '/') {
        // Absolute path
        size_t schemeEnd = url.find("://");
        if (schemeEnd != std::string::npos) {
            size_t hostEnd = url.find('/', schemeEnd + 3);
            return url.substr(0, hostEnd) + specifier;
        }
        return specifier;
    }
    
    if (specifier.substr(0, 2) == "./" || specifier.substr(0, 3) == "../") {
        // Relative path
        size_t lastSlash = url.rfind('/');
        std::string base = (lastSlash != std::string::npos) 
            ? url.substr(0, lastSlash + 1) 
            : "";
        return base + specifier;
    }
    
    // Bare specifier - return as-is (import maps would handle this)
    return specifier;
}

// =============================================================================
// WorkerModuleContext
// =============================================================================

WorkerModuleContext::WorkerModuleContext(WorkerType type, const std::string& scriptUrl)
    : type_(type)
    , scriptUrl_(scriptUrl)
    , loader_(Modules::ModuleLoaderHooks{})
    , linker_()
    , executor_(&linker_, nullptr) {
    
    importMeta_.url = scriptUrl;
}

void WorkerModuleContext::registerWithGC(GC::GCController* gc) {
    if (gc) {
        executor_.gcRoots();
        executor_.prepareForGC();
    }
}

// =============================================================================
// WorkerModuleLoader
// =============================================================================

WorkerModuleLoader::WorkerModuleLoader(WorkerType type)
    : context_(type, "") {}

std::future<bool> WorkerModuleLoader::loadEntryModule(const std::string& url, 
                                                       WorkerScriptType type) {
    return std::async(std::launch::async, [this, url, type]() {
        if (type == WorkerScriptType::Classic) {
            // Classic scripts use importScripts()
            return importScripts({url});
        }
        
        // Module script
        context_ = WorkerModuleContext(context_.workerType(), url);
        
        try {
            auto future = context_.loader().loadModule(url);
            entryModule_ = future.get();
            
            if (!entryModule_ || entryModule_->hasError()) {
                return false;
            }
            
            // Link
            if (!context_.executor().execute(entryModule_).isUndefined()) {
                // Execution error
            }
            
            ready_.store(true);
            return true;
        } catch (...) {
            return false;
        }
    });
}

std::future<Runtime::Value> WorkerModuleLoader::dynamicImport(const std::string& specifier) {
    return std::async(std::launch::async, [this, specifier]() -> Runtime::Value {
        // Resolve against current module
        std::string resolved = context_.importMeta().resolve(specifier);
        
        try {
            auto future = context_.loader().dynamicImport(resolved, context_.scriptUrl());
            auto* module = future.get();
            
            if (!module || module->hasError()) {
                throw std::runtime_error("Failed to import: " + specifier);
            }
            
            // Get namespace
            auto* env = context_.executor().getEnvironment(module);
            if (env) {
                return Runtime::Value::undefined(); // Would return namespace
            }
            
            return Runtime::Value::undefined();
        } catch (const std::exception& e) {
            throw;
        }
    });
}

bool WorkerModuleLoader::importScripts(const std::vector<std::string>& urls) {
    // Classic worker importScripts - synchronous loading and execution
    for (const auto& url : urls) {
        try {
            auto future = context_.loader().loadModule(url);
            auto* module = future.get();
            
            if (!module || module->hasError()) {
                return false;
            }
            
            context_.executor().execute(module);
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

bool WorkerModuleLoader::execute() {
    if (!entryModule_) return false;
    
    try {
        context_.executor().execute(entryModule_);
        return true;
    } catch (...) {
        return false;
    }
}

// =============================================================================
// SharedWorkerRegistry
// =============================================================================

SharedWorkerRegistry& SharedWorkerRegistry::instance() {
    static SharedWorkerRegistry instance;
    return instance;
}

WorkerModuleLoader* SharedWorkerRegistry::getOrCreate(const std::string& url, 
                                                       const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(url, name);
    auto it = workers_.find(key);
    
    if (it != workers_.end()) {
        return it->second.get();
    }
    
    auto loader = std::make_unique<WorkerModuleLoader>(WorkerType::SharedWorker);
    auto* ptr = loader.get();
    workers_[key] = std::move(loader);
    
    return ptr;
}

bool SharedWorkerRegistry::exists(const std::string& url, const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return workers_.find(makeKey(url, name)) != workers_.end();
}

void SharedWorkerRegistry::remove(const std::string& url, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    workers_.erase(makeKey(url, name));
}

std::string SharedWorkerRegistry::makeKey(const std::string& url, 
                                           const std::string& name) const {
    return url + "|" + name;
}

} // namespace Zepra::Workers
