/**
 * @file VMModuleIntegration.cpp
 * @brief VM-Module integration implementation
 */

#include "integration/VMModuleIntegration.h"

namespace Zepra::Integration {

// =============================================================================
// VMModuleIntegration
// =============================================================================

VMModuleIntegration::VMModuleIntegration(VM::Interpreter* interpreter,
                                          Modules::ModuleLoader* loader,
                                          Modules::ModuleExecutor* executor,
                                          GC::GCController* gc)
    : interpreter_(interpreter)
    , loader_(loader)
    , executor_(executor)
    , importHandler_(loader, executor)
    , gc_(gc) {}

Runtime::Value VMModuleIntegration::evaluateModule(Modules::ModuleRecord* module) {
    if (!module) {
        return Runtime::Value::undefined();
    }
    
    // Enter module context
    ModuleScope scope(*this, module);
    
    // Create module environment
    auto& state = currentState();
    state.moduleEnv = executor_->createEnvironment(module);
    
    // Create import.meta
    state.importMeta = new Modules::ImportMetaObject(module->specifier());
    
    // Register with GC
    registerModuleRoots(module);
    
    try {
        // Execute module via executor (handles compile+execute internally)
        auto result = executor_->execute(module);
        return result;
        
    } catch (const std::exception& e) {
        handleModuleException(Runtime::Value::undefined());
        throw;
    }
}

std::future<Runtime::Value> VMModuleIntegration::evaluateModuleAsync(Modules::ModuleRecord* module) {
    return std::async(std::launch::async, [this, module]() {
        return evaluateModule(module);
    });
}

Runtime::Value VMModuleIntegration::handleGetImport(const std::string& localName) {
    auto& state = currentState();
    
    if (!state.moduleEnv) {
        return Runtime::Value::undefined();
    }
    
    // Get binding from module environment
    return state.moduleEnv->getBindingValue(localName, true);
}

Runtime::Value VMModuleIntegration::handleDynamicImport(const std::string& specifier) {
    auto& state = currentState();
    std::string referrer = state.currentModule ? state.currentModule->specifier() : "";
    
    return importHandler_.handleImport(specifier, referrer);
}

Runtime::Value VMModuleIntegration::handleGetImportMeta() {
    auto& state = currentState();
    
    if (state.importMeta) {
        return state.importMeta->toValue();
    }
    
    return Runtime::Value::undefined();
}

void VMModuleIntegration::enterModule(Modules::ModuleRecord* module) {
    ModuleVMState state;
    state.currentModule = module;
    state.moduleEnv = executor_->getEnvironment(module);
    
    if (module) {
        state.importMeta = new Modules::ImportMetaObject(module->specifier());
    }
    
    moduleStack_.push_back(state);
}

void VMModuleIntegration::exitModule() {
    if (!moduleStack_.empty()) {
        auto& state = moduleStack_.back();
        
        // Cleanup import meta (GC will handle moduleEnv)
        delete state.importMeta;
        
        moduleStack_.pop_back();
    }
}

Modules::ModuleRecord* VMModuleIntegration::currentModule() const {
    if (moduleStack_.empty()) return nullptr;
    return moduleStack_.back().currentModule;
}

void VMModuleIntegration::handleModuleException(Runtime::Value exception) {
    auto* module = currentModule();
    
    if (module && executor_) {
        // Propagate error through module graph
        executor_->propagateError(module, "Module evaluation failed");
    }
}

Runtime::Value VMModuleIntegration::createModuleError(const std::string& message,
                                                       Modules::ModuleRecord* module) {
    // Would create SyntaxError or TypeError depending on context
    return Runtime::Value::undefined();
}

void VMModuleIntegration::registerModuleRoots(Modules::ModuleRecord* module) {
    if (!gc_) return;
    
    auto& state = currentState();
    
    if (state.moduleEnv) {
        executor_->gcRoots().registerEnvironment(state.moduleEnv);
    }
}

ModuleVMState& VMModuleIntegration::currentState() {
    static ModuleVMState empty;
    return moduleStack_.empty() ? empty : moduleStack_.back();
}

const ModuleVMState& VMModuleIntegration::currentState() const {
    static ModuleVMState empty;
    return moduleStack_.empty() ? empty : moduleStack_.back();
}

// =============================================================================
// Module Bytecode Helpers
// =============================================================================

namespace ModuleBytecode {

void emitGetImport(Bytecode::BytecodeGenerator& gen,
                   const std::string& localName,
                   const std::string& importedName) {
    // Emit: OP_GET_IMPORT <localName index>
    // Would emit bytecode to fetch import binding
}

void emitDynamicImport(Bytecode::BytecodeGenerator& gen) {
    // Emit: OP_DYNAMIC_IMPORT
    // Stack: specifier -> promise
}

void emitGetImportMeta(Bytecode::BytecodeGenerator& gen) {
    // Emit: OP_GET_IMPORT_META
    // Stack: -> import.meta object
}

void emitExportDefault(Bytecode::BytecodeGenerator& gen,
                       const std::string& localName) {
    // Emit: OP_EXPORT_DEFAULT <localName index>
}

} // namespace ModuleBytecode

} // namespace Zepra::Integration
