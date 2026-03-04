/**
 * @file VMModuleIntegration.h
 * @brief Integration between module system and VM
 * 
 * Implements:
 * - Module scope in VM
 * - Bytecode for import/export
 * - Module evaluation in VM
 * - Error handling integration
 */

#pragma once

#include "../modules/ModuleExecutor.h"
#include "../modules/DynamicImport.h"
#include "runtime/execution/Interpreter.h"
#include "runtime/execution/ExecutionContext.h"
#include "heap/GCController.h"

namespace Zepra::Bytecode { class BytecodeGenerator; }

namespace Zepra::Integration {

// =============================================================================
// Module Bytecode Operations
// =============================================================================

enum class ModuleBytecodeOp : uint8_t {
    // Import operations
    OP_GET_IMPORT,          // Get imported binding
    OP_GET_IMPORT_STAR,     // Get namespace import
    OP_DYNAMIC_IMPORT,      // import() expression
    
    // Export operations
    OP_EXPORT_DEFAULT,      // export default
    OP_EXPORT_NAMED,        // export { name }
    OP_EXPORT_ALL,          // export * from
    
    // Module meta
    OP_GET_IMPORT_META,     // import.meta
    OP_GET_MODULE_ENV       // Get module environment
};

// =============================================================================
// Module VM State
// =============================================================================

struct ModuleVMState {
    Modules::ModuleRecord* currentModule = nullptr;
    Modules::ModuleEnvironmentRecord* moduleEnv = nullptr;
    Modules::ImportMetaObject* importMeta = nullptr;
    
    // Pending dynamic imports
    std::vector<std::pair<std::string, Runtime::Value>> pendingImports;
};

// =============================================================================
// VM Module Integration
// =============================================================================

class VMModuleIntegration {
public:
    VMModuleIntegration(VM::Interpreter* interpreter,
                        Modules::ModuleLoader* loader,
                        Modules::ModuleExecutor* executor,
                        GC::GCController* gc);
    
    // =========================================================================
    // Module Evaluation
    // =========================================================================
    
    /**
     * @brief Evaluate module in VM
     */
    Runtime::Value evaluateModule(Modules::ModuleRecord* module);
    
    /**
     * @brief Evaluate module async (for TLA)
     */
    std::future<Runtime::Value> evaluateModuleAsync(Modules::ModuleRecord* module);
    
    // =========================================================================
    // Bytecode Handlers
    // =========================================================================
    
    /**
     * @brief Handle GET_IMPORT bytecode
     */
    Runtime::Value handleGetImport(const std::string& localName);
    
    /**
     * @brief Handle DYNAMIC_IMPORT bytecode
     */
    Runtime::Value handleDynamicImport(const std::string& specifier);
    
    /**
     * @brief Handle GET_IMPORT_META bytecode
     */
    Runtime::Value handleGetImportMeta();
    
    // =========================================================================
    // Module Context
    // =========================================================================
    
    /**
     * @brief Enter module evaluation context
     */
    void enterModule(Modules::ModuleRecord* module);
    
    /**
     * @brief Exit module evaluation context
     */
    void exitModule();
    
    /**
     * @brief Get current module
     */
    Modules::ModuleRecord* currentModule() const;
    
    // =========================================================================
    // Error Integration
    // =========================================================================
    
    /**
     * @brief Handle VM exception during module eval
     */
    void handleModuleException(Runtime::Value exception);
    
    /**
     * @brief Convert module error to VM exception
     */
    Runtime::Value createModuleError(const std::string& message, 
                                      Modules::ModuleRecord* module);
    
    // =========================================================================
    // GC Integration
    // =========================================================================
    
    /**
     * @brief Register module roots with GC
     */
    void registerModuleRoots(Modules::ModuleRecord* module);
    
    /**
     * @brief Visit module roots during GC
     */
    template<typename Visitor>
    void visitRoots(Visitor&& visitor) {
        for (auto& state : moduleStack_) {
            if (state.moduleEnv) {
                visitor(state.moduleEnv);
            }
            if (state.importMeta) {
                visitor(state.importMeta);
            }
        }
    }
    
private:
    VM::Interpreter* interpreter_;
    Modules::ModuleLoader* loader_;
    Modules::ModuleExecutor* executor_;
    Modules::DynamicImportHandler importHandler_;
    GC::GCController* gc_;
    
    // Module evaluation stack
    std::vector<ModuleVMState> moduleStack_;
    
    ModuleVMState& currentState();
    const ModuleVMState& currentState() const;
};

// =============================================================================
// Module Scope
// =============================================================================

class ModuleScope {
public:
    ModuleScope(VMModuleIntegration& integration, Modules::ModuleRecord* module)
        : integration_(integration) {
        integration_.enterModule(module);
    }
    
    ~ModuleScope() {
        integration_.exitModule();
    }
    
private:
    VMModuleIntegration& integration_;
};

// =============================================================================
// Module Bytecode Generator Helpers
// =============================================================================

namespace ModuleBytecode {

/**
 * @brief Emit bytecode for import binding access
 */
void emitGetImport(Bytecode::BytecodeGenerator& gen,
                   const std::string& localName,
                   const std::string& importedName);

/**
 * @brief Emit bytecode for dynamic import
 */
void emitDynamicImport(Bytecode::BytecodeGenerator& gen);

/**
 * @brief Emit bytecode for import.meta
 */
void emitGetImportMeta(Bytecode::BytecodeGenerator& gen);

/**
 * @brief Emit bytecode for export default
 */
void emitExportDefault(Bytecode::BytecodeGenerator& gen,
                       const std::string& localName);

} // namespace ModuleBytecode

} // namespace Zepra::Integration
