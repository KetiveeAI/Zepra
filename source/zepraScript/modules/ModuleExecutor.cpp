/**
 * @file ModuleExecutor.cpp
 * @brief Module execution implementation
 * 
 * Real execution semantics, not just headers.
 */

#include "modules/ModuleExecutor.h"
#include <algorithm>
#include <queue>
#include <stdexcept>

namespace Zepra::Modules {

// =============================================================================
// ModuleExecutor Implementation
// =============================================================================

ModuleExecutor::ModuleExecutor(ModuleLinker* linker, GC::GCController* gc)
    : linker_(linker), gcRoots_(gc) {}

Runtime::Value ModuleExecutor::execute(ModuleRecord* module) {
    if (!module) {
        return Runtime::Value::undefined();
    }
    
    // Check if already evaluated
    auto& state = getState(module);
    if (state.phase == EvaluationState::Phase::Evaluated) {
        return Runtime::Value::undefined();
    }
    
    if (state.phase == EvaluationState::Phase::Error) {
        // Re-throw stored error
        if (state.errorValue) {
            throw *state.errorValue;
        }
        return Runtime::Value::undefined();
    }
    
    // Get evaluation order from linker
    executionOrder_ = ExecutionHelpers::getEvaluationOrder(module, linker_);
    
    // Execute in order
    size_t index = 0;
    try {
        for (auto* mod : executionOrder_) {
            innerEvaluate(mod, index);
        }
    } catch (const std::exception& e) {
        propagateError(module, e.what());
        throw;
    }
    
    return Runtime::Value::undefined();
}

std::future<Runtime::Value> ModuleExecutor::executeAsync(ModuleRecord* module) {
    // Create capability for async execution
    auto cap = std::make_unique<ModuleCapability>();
    cap->module = module;
    cap->future = cap->promise.get_future();
    
    auto* capPtr = cap.get();
    capabilities_[module] = std::move(cap);
    
    // Start async evaluation
    scheduleAsyncEvaluation(module);
    
    return std::move(capPtr->future);
}

bool ModuleExecutor::executeAll(const std::vector<ModuleRecord*>& modules) {
    bool allSuccess = true;
    
    for (auto* mod : modules) {
        try {
            execute(mod);
        } catch (...) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

Runtime::Value ModuleExecutor::innerEvaluate(ModuleRecord* module, size_t& index) {
    auto& state = getState(module);
    
    // Already evaluated?
    if (state.phase == EvaluationState::Phase::Evaluated) {
        return Runtime::Value::undefined();
    }
    
    // Error in this module?
    if (state.phase == EvaluationState::Phase::Error) {
        if (state.errorValue) {
            throw *state.errorValue;
        }
        return Runtime::Value::undefined();
    }
    
    // Currently evaluating? (circular dependency handled by linking)
    if (state.phase == EvaluationState::Phase::Evaluating) {
        return Runtime::Value::undefined();
    }
    
    // Mark as evaluating
    state.phase = EvaluationState::Phase::Evaluating;
    
    // Create environment if not exists
    auto* env = createEnvironment(module);
    
    // Register as GC root
    gcRoots_.registerEnvironment(env);
    
    // Evaluate dependencies first
    for (const auto& depSpec : module->requestedModules()) {
        auto* dep = module->getResolvedModule(depSpec);
        if (dep) {
            innerEvaluate(dep, index);
        }
    }
    
    // Check if any dependency has TLA
    if (dependsOnTopLevelAwait(module)) {
        state.hasTopLevelAwait = true;
        scheduleAsyncEvaluation(module);
        state.phase = EvaluationState::Phase::EvaluatedAsync;
        return Runtime::Value::undefined();
    }
    
    // Execute module body
    try {
        auto result = executeModuleBody(module);
        state.phase = EvaluationState::Phase::Evaluated;
        return result;
    } catch (const std::exception& e) {
        state.phase = EvaluationState::Phase::Error;
        state.error = e.what();
        propagateError(module, e.what());
        throw;
    }
}

Runtime::Value ModuleExecutor::executeModuleBody(ModuleRecord* module) {
    auto* env = getEnvironment(module);
    
    if (evalCallback_) {
        return evalCallback_(module, env);
    }
    
    // Default: just mark as evaluated
    return Runtime::Value::undefined();
}

// =============================================================================
// Top-Level Await
// =============================================================================

bool ModuleExecutor::hasTopLevelAwait(ModuleRecord* module) const {
    auto it = states_.find(module);
    return it != states_.end() && it->second.hasTopLevelAwait;
}

bool ModuleExecutor::dependsOnTopLevelAwait(ModuleRecord* module) const {
    // Check all dependencies recursively
    std::queue<ModuleRecord*> toCheck;
    std::unordered_set<ModuleRecord*> visited;
    
    for (const auto& depSpec : module->requestedModules()) {
        if (auto* dep = module->getResolvedModule(depSpec)) {
            toCheck.push(dep);
        }
    }
    
    while (!toCheck.empty()) {
        auto* current = toCheck.front();
        toCheck.pop();
        
        if (visited.count(current)) continue;
        visited.insert(current);
        
        if (hasTopLevelAwait(current)) {
            return true;
        }
        
        // Check if still evaluating async
        auto it = states_.find(current);
        if (it != states_.end() && 
            it->second.phase == EvaluationState::Phase::EvaluatedAsync) {
            return true;
        }
        
        for (const auto& depSpec : current->requestedModules()) {
            if (auto* dep = current->getResolvedModule(depSpec)) {
                toCheck.push(dep);
            }
        }
    }
    
    return false;
}

void ModuleExecutor::scheduleAsyncEvaluation(ModuleRecord* module) {
    auto& state = getState(module);
    state.phase = EvaluationState::Phase::EvaluatedAsync;
    
    // Gather async parent modules
    gatherAsyncParents(module);
    
    // Create environment
    auto* env = createEnvironment(module);
    gcRoots_.registerEnvironment(env);
    
    // The actual async evaluation will be triggered when all
    // async dependencies resolve
}

void ModuleExecutor::gatherAsyncParents(ModuleRecord* module) {
    // Find all modules that depend on this module
    const auto& graph = linker_->graph();
    auto dependents = graph.getDependents(module);
    
    for (auto* dep : dependents) {
        auto it = capabilities_.find(dep);
        if (it != capabilities_.end()) {
            it->second->asyncParents.push_back(capabilities_[module].get());
            capabilities_[module]->pendingAsyncDeps++;
        }
    }
}

void ModuleExecutor::onAsyncEvaluationComplete(ModuleRecord* module, Runtime::Value result) {
    auto& state = getState(module);
    state.phase = EvaluationState::Phase::Evaluated;
    
    // Resolve capability promise
    auto it = capabilities_.find(module);
    if (it != capabilities_.end()) {
        it->second->promise.set_value(result);
        
        // Notify waiting parents
        for (auto* parent : it->second->asyncParents) {
            parent->pendingAsyncDeps--;
            if (parent->pendingAsyncDeps == 0) {
                // All async deps resolved, continue parent evaluation
                size_t index = 0;
                try {
                    innerEvaluate(parent->module, index);
                } catch (...) {
                    // Error handled by innerEvaluate
                }
            }
        }
    }
}

void ModuleExecutor::onAsyncEvaluationError(ModuleRecord* module, Runtime::Value error) {
    auto& state = getState(module);
    state.phase = EvaluationState::Phase::Error;
    state.errorValue = error;
    
    // Reject capability promise
    auto it = capabilities_.find(module);
    if (it != capabilities_.end()) {
        try {
            it->second->promise.set_exception(
                std::make_exception_ptr(std::runtime_error("Module evaluation failed")));
        } catch (...) {}
    }
    
    // Propagate error to dependents
    propagateError(module, "Async module evaluation failed");
}

// =============================================================================
// Error Propagation
// =============================================================================

void ModuleExecutor::propagateError(ModuleRecord* failedModule, const std::string& error) {
    // Get all modules that depend on the failed module
    const auto& graph = linker_->graph();
    std::queue<ModuleRecord*> toPropagate;
    std::unordered_set<ModuleRecord*> propagated;
    
    // Mark source as error
    auto& sourceState = getState(failedModule);
    sourceState.phase = EvaluationState::Phase::Error;
    sourceState.error = error;
    propagated.insert(failedModule);
    
    // Find all dependents
    auto dependents = graph.getDependents(failedModule);
    for (auto* dep : dependents) {
        toPropagate.push(dep);
    }
    
    // Propagate through graph
    while (!toPropagate.empty()) {
        auto* current = toPropagate.front();
        toPropagate.pop();
        
        if (propagated.count(current)) continue;
        propagated.insert(current);
        
        auto& state = getState(current);
        if (state.phase != EvaluationState::Phase::Error) {
            state.phase = EvaluationState::Phase::Error;
            state.error = "Dependency failed: " + failedModule->specifier() + ": " + error;
        }
        
        // Reject any pending promises
        auto it = capabilities_.find(current);
        if (it != capabilities_.end()) {
            try {
                it->second->promise.set_exception(
                    std::make_exception_ptr(std::runtime_error(state.error.value_or("Unknown error"))));
            } catch (...) {}
        }
        
        // Continue propagating
        for (auto* dep : graph.getDependents(current)) {
            toPropagate.push(dep);
        }
    }
}

std::optional<std::string> ModuleExecutor::getError(ModuleRecord* module) const {
    auto it = states_.find(module);
    if (it != states_.end() && it->second.phase == EvaluationState::Phase::Error) {
        return it->second.error;
    }
    return std::nullopt;
}

bool ModuleExecutor::hasAnyError(ModuleRecord* module) const {
    if (getError(module)) return true;
    
    // Check dependencies
    for (const auto& depSpec : module->requestedModules()) {
        if (auto* dep = module->getResolvedModule(depSpec)) {
            if (hasAnyError(dep)) return true;
        }
    }
    
    return false;
}

// =============================================================================
// Environment
// =============================================================================

ModuleEnvironmentRecord* ModuleExecutor::getEnvironment(ModuleRecord* module) {
    auto it = environments_.find(module);
    return it != environments_.end() ? it->second.get() : nullptr;
}

ModuleEnvironmentRecord* ModuleExecutor::createEnvironment(ModuleRecord* module) {
    auto it = environments_.find(module);
    if (it != environments_.end()) {
        return it->second.get();
    }
    
    auto env = std::make_unique<ModuleEnvironmentRecord>(module);
    auto* ptr = env.get();
    environments_[module] = std::move(env);
    
    // Initialize import bindings
    for (const auto& import : module->imports()) {
        auto* sourceModule = module->getResolvedModule(import.moduleRequest);
        if (sourceModule) {
            ptr->createImportBinding(import.localName, sourceModule, import.importName);
        }
    }
    
    return ptr;
}

// =============================================================================
// GC Integration
// =============================================================================

void ModuleExecutor::prepareForGC() {
    // All namespaces and environments are already registered
    // This is called before GC to ensure nothing is missed
}

EvaluationState& ModuleExecutor::getState(ModuleRecord* module) {
    return states_[module];
}

const EvaluationState& ModuleExecutor::getState(ModuleRecord* module) const {
    static EvaluationState empty;
    auto it = states_.find(module);
    return it != states_.end() ? it->second : empty;
}

// =============================================================================
// Helpers
// =============================================================================

namespace ExecutionHelpers {

Runtime::Value createModulePromise() {
    // Would integrate with Promise implementation
    return Runtime::Value::undefined();
}

void resolveModulePromise(Runtime::Value promise, Runtime::Value value) {
    // Would call Promise.resolve
}

void rejectModulePromise(Runtime::Value promise, Runtime::Value error) {
    // Would call Promise.reject
}

std::vector<ModuleRecord*> getEvaluationOrder(ModuleRecord* entry, ModuleLinker* linker) {
    if (!linker) return {entry};
    return linker->graph().topologicalSort();
}

} // namespace ExecutionHelpers

} // namespace Zepra::Modules
