/**
 * @file module.cpp
 * @brief ES6 Module System Implementation
 * 
 * Note: Full module parsing requires integration with the bytecode compiler.
 * This provides the module record and loader infrastructure.
 */

#include "runtime/handles/module.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/async/promise.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Zepra::Runtime {

// ============================================================================
// Module
// ============================================================================

Module::Module(const std::string& specifier)
    : specifier_(specifier)
    , status_(ModuleStatus::Unlinked)
    , namespace_(nullptr)
    , environment_(nullptr) {}

void Module::addImport(const ImportEntry& entry) {
    imports_.push_back(entry);
}

void Module::addExport(const ExportEntry& entry) {
    exports_.push_back(entry);
}

Value Module::getExport(const std::string& name) const {
    auto it = exportedBindings_.find(name);
    if (it != exportedBindings_.end()) {
        return it->second;
    }
    return Value::undefined();
}

void Module::setExport(const std::string& name, Value value) {
    exportedBindings_[name] = value;
}

Object* Module::getNamespace() {
    if (!namespace_) {
        namespace_ = new Object();
        for (const auto& [name, value] : exportedBindings_) {
            namespace_->set(name, value);
        }
    }
    return namespace_;
}

// ============================================================================
// ModuleLoader - DISABLED: Implementation moved to module_loader.cpp
// ============================================================================

/* Disabled - duplicate definition in module_loader.cpp
ModuleLoader::ModuleLoader() {}

ModuleLoader& ModuleLoader::instance() {
    static ModuleLoader instance;
    return instance;
}

Module* ModuleLoader::loadModule(const std::string& specifier, const std::string& referrer) {
    std::string resolved = resolveModuleSpecifier(specifier, referrer);
    
    // Check cache
    auto it = moduleCache_.find(resolved);
    if (it != moduleCache_.end()) {
        return it->second.get();
    }
    
    // Read module source
    std::string source = readModuleSource(resolved);
    if (source.empty()) {
        return nullptr;
    }
    
    // Create module record
    // Note: Full parsing would happen here with the bytecode compiler
    // For now, we just create the module record
    auto module = std::make_unique<Module>(resolved);
    module->setStatus(ModuleStatus::Linked);
    
    Module* modulePtr = module.get();
    moduleCache_[resolved] = std::move(module);
    
    return modulePtr;
}
*/

Module* ModuleLoader::getModule(const std::string& specifier) const {
    auto it = moduleCache_.find(specifier);
    if (it != moduleCache_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::string ModuleLoader::resolveModuleSpecifier(const std::string& specifier, const std::string& referrer) {
    // Handle relative specifiers
    if (specifier.length() >= 2 && (specifier[0] == '.' && (specifier[1] == '/' || 
        (specifier[1] == '.' && specifier.length() >= 3 && specifier[2] == '/')))) {
        size_t lastSlash = referrer.rfind('/');
        std::string baseDir = (lastSlash != std::string::npos) 
            ? referrer.substr(0, lastSlash + 1) 
            : "";
        return baseDir + specifier;
    }
    
    // Handle bare specifiers (node_modules style or built-in)
    return specifier;
}

std::string ModuleLoader::readModuleSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool ModuleLoader::linkModule(Module* module) {
    if (module->status() != ModuleStatus::Unlinked) {
        return module->status() == ModuleStatus::Linked || 
               module->status() == ModuleStatus::Evaluated;
    }
    
    module->setStatus(ModuleStatus::Linking);
    
    // Resolve all imports
    for (const auto& importEntry : module->imports()) {
        Module* imported = loadModule(importEntry.moduleRequest, module->specifier());
        if (!imported) {
            module->setStatus(ModuleStatus::Unlinked);
            return false;
        }
        
        // Recursively link
        if (!linkModule(imported)) {
            module->setStatus(ModuleStatus::Unlinked);
            return false;
        }
    }
    
    module->setStatus(ModuleStatus::Linked);
    return true;
}

bool ModuleLoader::evaluateModule(Module* module) {
    if (module->status() == ModuleStatus::Evaluated) {
        return true;
    }
    
    if (module->status() != ModuleStatus::Linked) {
        return false;
    }
    
    module->setStatus(ModuleStatus::Evaluating);
    
    // Evaluate all dependencies first
    for (const auto& importEntry : module->imports()) {
        Module* imported = getModule(importEntry.moduleRequest);
        if (imported && imported->status() == ModuleStatus::Linked) {
            if (!evaluateModule(imported)) {
                module->setStatus(ModuleStatus::Linked);
                return false;
            }
        }
    }
    
    // Module body execution would happen here via bytecode interpreter
    
    module->setStatus(ModuleStatus::Evaluated);
    return true;
}

// New methods

bool Module::hasExport(const std::string& name) const {
    return exportedBindings_.find(name) != exportedBindings_.end();
}

std::vector<std::string> Module::exportNames() const {
    std::vector<std::string> names;
    names.reserve(exportedBindings_.size());
    for (const auto& [name, _] : exportedBindings_) {
        names.push_back(name);
    }
    return names;
}

ImportMeta* Module::getImportMeta() {
    if (!importMeta_) {
        importMeta_ = new ImportMeta(specifier_);
    }
    return importMeta_;
}

ImportMeta::ImportMeta(const std::string& url)
    : url_(url)
{
    // Store url - accessed via url() method
}

Promise* ModuleLoader::dynamicImport(const std::string& specifier, const std::string& referrer) {
    // Create promise for async module loading
    Promise* promise = new Promise();
    
    try {
        Module* module = loadModule(specifier, referrer);
        if (!module) {
            promise->reject(Value::string(new String("Cannot find module: " + specifier)));
            return promise;
        }
        
        if (!linkModule(module)) {
            promise->reject(Value::string(new String("Failed to link module: " + specifier)));
            return promise;
        }
        
        if (!evaluateModule(module)) {
            promise->reject(Value::string(new String("Failed to evaluate module: " + specifier)));
            return promise;
        }
        
        // Resolve with module namespace
        promise->resolve(Value::object(module->getNamespace()));
    } catch (const std::exception& e) {
        promise->reject(Value::string(new String(std::string("Module load error: ") + e.what())));
    }
    
    return promise;
}

/* Disabled - duplicate definition in module_loader.cpp
void ModuleLoader::registerBuiltinModule(const std::string& name, Object* exports) {
    builtinModules_[name] = exports;
}
*/

} // namespace Zepra::Runtime

