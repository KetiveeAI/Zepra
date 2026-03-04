/**
 * @file ModuleExecutor.h
 * @brief Module execution semantics and evaluation
 * 
 * Implements:
 * - Module evaluation order
 * - Top-level await (TLA)
 * - Error propagation across graph
 * - GC root registration
 * 
 * ES2022 Module execution spec
 */

#pragma once

#include "ModuleLoader.h"
#include "ModuleLinker.h"
#include "ModuleNamespace.h"
#include "runtime/objects/value.hpp"
#include "heap/GCController.h"
#include <future>
#include <optional>

namespace Zepra::Modules {

// =============================================================================
// Evaluation State
// =============================================================================

struct EvaluationState {
    enum class Phase {
        Pending,
        Evaluating,
        EvaluatedAsync,     // TLA: waiting on promises
        Evaluated,
        Error
    };
    
    Phase phase = Phase::Pending;
    std::optional<std::string> error;
    std::optional<Runtime::Value> errorValue;
    
    // For TLA
    std::vector<std::promise<void>*> pendingPromises;
    size_t asyncParentCount = 0;
    bool hasTopLevelAwait = false;
    
    bool isComplete() const {
        return phase == Phase::Evaluated || phase == Phase::Error;
    }
};

// =============================================================================
// Module Capability Record (for TLA)
// =============================================================================

struct ModuleCapability {
    ModuleRecord* module;
    std::promise<Runtime::Value> promise;
    std::future<Runtime::Value> future;
    
    // Tracking async dependencies
    size_t pendingAsyncDeps = 0;
    std::vector<ModuleCapability*> asyncParents;
};

// =============================================================================
// GC Root Handle
// =============================================================================

class ModuleGCRoots {
public:
    explicit ModuleGCRoots(GC::GCController* gc) : gc_(gc) {}
    
    /**
     * @brief Register module namespace as GC root
     */
    void registerNamespace(ModuleNamespace* ns) {
        namespaces_.push_back(ns);
        // Register with GC as root
        if (gc_) {
            // TODO: gc_->addRoot when GCController supports it
        }
    }
    
    /**
     * @brief Register module environment as GC root
     */
    void registerEnvironment(ModuleEnvironmentRecord* env) {
        environments_.push_back(env);
        if (gc_) {
            // TODO: gc_->addRoot when GCController supports it
        }
    }
    
    /**
     * @brief Unregister all roots for module
     */
    void unregisterModule(ModuleRecord* module);
    
    /**
     * @brief Visit all module roots (for GC marking)
     */
    template<typename Visitor>
    void visitRoots(Visitor&& visitor) {
        for (auto* ns : namespaces_) {
            visitor(ns);
        }
        for (auto* env : environments_) {
            visitor(env);
        }
    }
    
private:
    GC::GCController* gc_;
    std::vector<ModuleNamespace*> namespaces_;
    std::vector<ModuleEnvironmentRecord*> environments_;
};

// =============================================================================
// Module Executor
// =============================================================================

class ModuleExecutor {
public:
    using EvalCallback = std::function<Runtime::Value(ModuleRecord*, ModuleEnvironmentRecord*)>;
    
    ModuleExecutor(ModuleLinker* linker, GC::GCController* gc = nullptr);
    
    // =========================================================================
    // Execution
    // =========================================================================
    
    /**
     * @brief Execute a linked module (sync, no TLA)
     */
    Runtime::Value execute(ModuleRecord* module);
    
    /**
     * @brief Execute module with TLA support
     */
    std::future<Runtime::Value> executeAsync(ModuleRecord* module);
    
    /**
     * @brief Execute all modules in graph order
     */
    bool executeAll(const std::vector<ModuleRecord*>& modules);
    
    // =========================================================================
    // Top-Level Await
    // =========================================================================
    
    /**
     * @brief Check if module has TLA
     */
    bool hasTopLevelAwait(ModuleRecord* module) const;
    
    /**
     * @brief Check if any dependency has TLA
     */
    bool dependsOnTopLevelAwait(ModuleRecord* module) const;
    
    /**
     * @brief Handle TLA promise resolution
     */
    void onAsyncEvaluationComplete(ModuleRecord* module, Runtime::Value result);
    
    /**
     * @brief Handle TLA promise rejection
     */
    void onAsyncEvaluationError(ModuleRecord* module, Runtime::Value error);
    
    // =========================================================================
    // Error Propagation
    // =========================================================================
    
    /**
     * @brief Propagate error through module graph
     */
    void propagateError(ModuleRecord* failedModule, const std::string& error);
    
    /**
     * @brief Get error for module (may be propagated)
     */
    std::optional<std::string> getError(ModuleRecord* module) const;
    
    /**
     * @brief Check if module or any dependency has error
     */
    bool hasAnyError(ModuleRecord* module) const;
    
    // =========================================================================
    // GC Integration
    // =========================================================================
    
    /**
     * @brief Get GC roots manager
     */
    ModuleGCRoots& gcRoots() { return gcRoots_; }
    
    /**
     * @brief Called before GC to ensure module roots are marked
     */
    void prepareForGC();
    
    // =========================================================================
    // Environment Access
    // =========================================================================
    
    /**
     * @brief Get environment for module
     */
    ModuleEnvironmentRecord* getEnvironment(ModuleRecord* module);
    
    /**
     * @brief Create environment for module
     */
    ModuleEnvironmentRecord* createEnvironment(ModuleRecord* module);
    
    // =========================================================================
    // Callbacks
    // =========================================================================
    
    void setEvalCallback(EvalCallback callback) { evalCallback_ = std::move(callback); }
    
    // =========================================================================
    // State
    // =========================================================================
    
    EvaluationState& getState(ModuleRecord* module);
    const EvaluationState& getState(ModuleRecord* module) const;
    
private:
    ModuleLinker* linker_;
    ModuleGCRoots gcRoots_;
    EvalCallback evalCallback_;
    
    std::unordered_map<ModuleRecord*, EvaluationState> states_;
    std::unordered_map<ModuleRecord*, std::unique_ptr<ModuleEnvironmentRecord>> environments_;
    std::unordered_map<ModuleRecord*, std::unique_ptr<ModuleCapability>> capabilities_;
    
    // Execution order from linker's topological sort
    std::vector<ModuleRecord*> executionOrder_;
    
    // Inner evaluation (spec: InnerModuleEvaluation)
    Runtime::Value innerEvaluate(ModuleRecord* module, size_t& index);
    
    // Execute module body
    Runtime::Value executeModuleBody(ModuleRecord* module);
    
    // Handle async module evaluation
    void scheduleAsyncEvaluation(ModuleRecord* module);
    void gatherAsyncParents(ModuleRecord* module);
};

// =============================================================================
// Execution Helpers
// =============================================================================

namespace ExecutionHelpers {

/**
 * @brief Create promise for async module
 */
Runtime::Value createModulePromise();

/**
 * @brief Resolve module promise
 */
void resolveModulePromise(Runtime::Value promise, Runtime::Value value);

/**
 * @brief Reject module promise with error
 */
void rejectModulePromise(Runtime::Value promise, Runtime::Value error);

/**
 * @brief Get all modules in evaluation order
 */
std::vector<ModuleRecord*> getEvaluationOrder(ModuleRecord* entry, ModuleLinker* linker);

} // namespace ExecutionHelpers

} // namespace Zepra::Modules
