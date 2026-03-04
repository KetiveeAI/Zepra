/**
 * @file ModuleLinker.cpp
 * @brief Module linking implementation
 */

#include "modules/ModuleLinker.h"
#include <algorithm>

namespace Zepra::Modules {

// =============================================================================
// Module Graph
// =============================================================================

void ModuleGraph::addModule(ModuleRecord* module) {
    if (std::find(modules_.begin(), modules_.end(), module) == modules_.end()) {
        modules_.push_back(module);
    }
}

void ModuleGraph::addEdge(ModuleRecord* from, ModuleRecord* to) {
    edges_[from].push_back(to);
}

std::vector<ModuleRecord*> ModuleGraph::topologicalSort() const {
    std::vector<ModuleRecord*> result;
    std::unordered_set<ModuleRecord*> visited;
    std::unordered_set<ModuleRecord*> onStack;
    
    std::function<void(ModuleRecord*)> visit = [&](ModuleRecord* node) {
        if (visited.count(node)) return;
        if (onStack.count(node)) return; // Cycle, skip
        
        onStack.insert(node);
        
        auto it = edges_.find(node);
        if (it != edges_.end()) {
            for (auto* dep : it->second) {
                visit(dep);
            }
        }
        
        onStack.erase(node);
        visited.insert(node);
        result.push_back(node);
    };
    
    for (auto* mod : modules_) {
        visit(mod);
    }
    
    return result;
}

bool ModuleGraph::hasCircularDependency() const {
    std::unordered_set<ModuleRecord*> visited;
    std::unordered_set<ModuleRecord*> onStack;
    std::vector<ModuleRecord*> path;
    
    for (auto* mod : modules_) {
        if (hasCycleFrom(mod, visited, onStack, path)) {
            return true;
        }
    }
    
    return false;
}

std::vector<ModuleRecord*> ModuleGraph::getCircularPath() const {
    std::unordered_set<ModuleRecord*> visited;
    std::unordered_set<ModuleRecord*> onStack;
    std::vector<ModuleRecord*> path;
    
    for (auto* mod : modules_) {
        if (hasCycleFrom(mod, visited, onStack, path)) {
            return path;
        }
    }
    
    return {};
}

std::vector<ModuleRecord*> ModuleGraph::getDependents(ModuleRecord* module) const {
    std::vector<ModuleRecord*> result;
    
    for (const auto& [from, deps] : edges_) {
        if (std::find(deps.begin(), deps.end(), module) != deps.end()) {
            result.push_back(from);
        }
    }
    
    return result;
}

bool ModuleGraph::hasCycleFrom(ModuleRecord* start,
                                std::unordered_set<ModuleRecord*>& visited,
                                std::unordered_set<ModuleRecord*>& onStack,
                                std::vector<ModuleRecord*>& path) const {
    if (onStack.count(start)) {
        path.push_back(start);
        return true;
    }
    if (visited.count(start)) return false;
    
    visited.insert(start);
    onStack.insert(start);
    path.push_back(start);
    
    auto it = edges_.find(start);
    if (it != edges_.end()) {
        for (auto* dep : it->second) {
            if (hasCycleFrom(dep, visited, onStack, path)) {
                return true;
            }
        }
    }
    
    onStack.erase(start);
    path.pop_back();
    return false;
}

// =============================================================================
// Module Linker
// =============================================================================

bool ModuleLinker::link(ModuleRecord* module) {
    if (!module) return false;
    
    if (module->status() == ModuleStatus::Linked) {
        return true;
    }
    
    if (module->status() == ModuleStatus::Linking) {
        // Circular dependency - allowed but noted
        return true;
    }
    
    clearErrors();
    
    std::stack<ModuleRecord*> stack;
    size_t index = 0;
    
    return innerModuleLinking(module, stack, index);
}

bool ModuleLinker::linkAll(const std::vector<ModuleRecord*>& modules) {
    bool allSuccess = true;
    
    for (auto* mod : modules) {
        if (!link(mod)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool ModuleLinker::innerModuleLinking(ModuleRecord* module,
                                      std::stack<ModuleRecord*>& stack,
                                      size_t& index) {
    if (module->status() == ModuleStatus::Linking) {
        return true; // Circular, already being linked
    }
    
    if (module->status() >= ModuleStatus::Linked) {
        return true; // Already linked
    }
    
    module->setStatus(ModuleStatus::Linking);
    graph_.addModule(module);
    stack.push(module);
    
    // Link dependencies first
    for (const auto& depSpec : module->requestedModules()) {
        auto* dep = module->getResolvedModule(depSpec);
        if (!dep) {
            addError(LinkError::Type::UnresolvedImport,
                     "Cannot resolve module: " + depSpec,
                     module->specifier(), depSpec);
            module->setStatus(ModuleStatus::Unlinked);
            return false;
        }
        
        graph_.addEdge(module, dep);
        
        if (!innerModuleLinking(dep, stack, index)) {
            module->setStatus(ModuleStatus::Unlinked);
            return false;
        }
    }
    
    // Resolve all imports
    if (!resolveAllImports(module)) {
        module->setStatus(ModuleStatus::Unlinked);
        return false;
    }
    
    // Mark as linked
    module->setStatus(ModuleStatus::Linked);
    stack.pop();
    
    return true;
}

ResolvedBinding ModuleLinker::resolveExport(ModuleRecord* module,
                                             const std::string& exportName,
                                             std::unordered_set<ModuleRecord*>& resolveSet) {
    if (resolveSet.count(module)) {
        // Circular
        return ResolvedBinding{nullptr, "", false, false};
    }
    resolveSet.insert(module);
    
    for (const auto& exp : module->exports()) {
        if (exp.exportName == exportName) {
            if (!exp.isReExport()) {
                // Local export
                return ResolvedBinding{module, exp.localName, false, false};
            } else {
                // Re-export
                auto* sourceModule = module->getResolvedModule(exp.moduleRequest);
                if (!sourceModule) {
                    return ResolvedBinding{nullptr, "", false, false};
                }
                
                if (exp.importName == "*") {
                    // Namespace re-export
                    return ResolvedBinding{sourceModule, "", false, true};
                }
                
                return resolveExport(sourceModule, exp.importName, resolveSet);
            }
        }
    }
    
    // Check star exports
    ResolvedBinding starResolution{nullptr, "", false, false};
    for (const auto& exp : module->exports()) {
        if (exp.isNamespaceExport()) {
            auto* starModule = module->getResolvedModule(exp.moduleRequest);
            if (starModule) {
                auto resolution = resolveExport(starModule, exportName, resolveSet);
                if (resolution.module) {
                    if (starResolution.module && 
                        starResolution.module != resolution.module) {
                        // Ambiguous
                        return ResolvedBinding{nullptr, "", true, false};
                    }
                    starResolution = resolution;
                }
            }
        }
    }
    
    return starResolution;
}

std::vector<std::string> ModuleLinker::getExportedNames(ModuleRecord* module,
                                                         std::unordered_set<ModuleRecord*>& exportStars) {
    std::vector<std::string> names;
    
    if (exportStars.count(module)) {
        return names; // Circular
    }
    exportStars.insert(module);
    
    for (const auto& exp : module->exports()) {
        if (exp.isNamespaceExport()) {
            // Star export - get all names from source
            auto* sourceModule = module->getResolvedModule(exp.moduleRequest);
            if (sourceModule) {
                auto sourceNames = getExportedNames(sourceModule, exportStars);
                for (const auto& name : sourceNames) {
                    if (name != "default" && 
                        std::find(names.begin(), names.end(), name) == names.end()) {
                        names.push_back(name);
                    }
                }
            }
        } else {
            names.push_back(exp.exportName);
        }
    }
    
    return names;
}

ResolvedBinding ModuleLinker::resolveImport(ModuleRecord* module, const ImportEntry& import) {
    auto* sourceModule = module->getResolvedModule(import.moduleRequest);
    if (!sourceModule) {
        addError(LinkError::Type::UnresolvedImport,
                 "Cannot resolve module: " + import.moduleRequest,
                 module->specifier(), import.importName);
        return ResolvedBinding{nullptr, "", false, false};
    }
    
    if (import.importName == "*") {
        // Namespace import
        return ResolvedBinding{sourceModule, "", false, true};
    }
    
    std::unordered_set<ModuleRecord*> resolveSet;
    return resolveExport(sourceModule, import.importName, resolveSet);
}

bool ModuleLinker::resolveAllImports(ModuleRecord* module) {
    for (const auto& import : module->imports()) {
        auto binding = resolveImport(module, import);
        
        if (binding.isAmbiguous) {
            addError(LinkError::Type::AmbiguousExport,
                     "Ambiguous export: " + import.importName,
                     module->specifier(), import.importName);
            return false;
        }
        
        if (!binding.module && !binding.isNamespace) {
            addError(LinkError::Type::UnresolvedImport,
                     "Export not found: " + import.importName,
                     import.moduleRequest, import.importName);
            return false;
        }
    }
    
    return true;
}

void ModuleLinker::addError(LinkError::Type type,
                            const std::string& message,
                            const std::string& module,
                            const std::string& name) {
    errors_.push_back({type, message, module, name});
}

std::string LinkError::format() const {
    return "[" + moduleSpecifier + "] " + message;
}

} // namespace Zepra::Modules
