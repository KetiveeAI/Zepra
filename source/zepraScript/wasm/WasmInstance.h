/**
 * @file WasmInstance.h
 * @brief WebAssembly instance implementation
 * 
 * Provides instance management for WASM including:
 * - Memory, table, global management
 * - Function exports/imports
 * - Module instantiation
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC
 */

#pragma once

#include "WasmConstants.h"
#include "WasmTypeDef.h"
#include "WasmMemory.h"
#include "WasmTable.h"
#include "WasmGlobal.h"
#include "ZWasmTierController.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

namespace Zepra::Wasm {

// Forward declarations
class WasmModule;
class WasmCodeBlock;

// =============================================================================
// Function Reference (callable from WASM or host)
// =============================================================================

struct WasmFunction {
    enum class Kind : uint8_t {
        Wasm,       // WASM bytecode function
        Host,       // Host-provided function
        Compiled    // JIT-compiled function
    };
    
    Kind kind = Kind::Wasm;
    uint32_t typeIndex = 0;         // Index into type section
    uint32_t funcIndex = 0;         // Index in module
    
    // For WASM functions
    uint32_t codeOffset = 0;        // Offset in code section
    uint32_t codeSize = 0;          // Size of function body
    
    // For compiled functions
    void* compiledCode = nullptr;   // Pointer to JIT code
    
    // For host functions
    using HostCallback = std::function<WasmValue(const std::vector<WasmValue>&)>;
    HostCallback hostFunc;
    
    // Tiering info (stored as pointer to avoid move issues with atomics)
    std::unique_ptr<ZTierController> tieringInfo;
    
    WasmFunction() : tieringInfo(std::make_unique<ZTierController>()) {}
    
    // Move constructor
    WasmFunction(WasmFunction&& other) noexcept
        : kind(other.kind)
        , typeIndex(other.typeIndex)
        , funcIndex(other.funcIndex)
        , codeOffset(other.codeOffset)
        , codeSize(other.codeSize)
        , compiledCode(other.compiledCode)
        , hostFunc(std::move(other.hostFunc))
        , tieringInfo(std::move(other.tieringInfo)) {}
    
    // Move assignment
    WasmFunction& operator=(WasmFunction&& other) noexcept {
        if (this != &other) {
            kind = other.kind;
            typeIndex = other.typeIndex;
            funcIndex = other.funcIndex;
            codeOffset = other.codeOffset;
            codeSize = other.codeSize;
            compiledCode = other.compiledCode;
            hostFunc = std::move(other.hostFunc);
            tieringInfo = std::move(other.tieringInfo);
        }
        return *this;
    }
    
    // Delete copy (due to unique_ptr)
    WasmFunction(const WasmFunction&) = delete;
    WasmFunction& operator=(const WasmFunction&) = delete;
    
    static WasmFunction wasm(uint32_t typeIdx, uint32_t funcIdx, uint32_t offset, uint32_t size) {
        WasmFunction f;
        f.kind = Kind::Wasm;
        f.typeIndex = typeIdx;
        f.funcIndex = funcIdx;
        f.codeOffset = offset;
        f.codeSize = size;
        return f;
    }
    
    static WasmFunction host(uint32_t typeIdx, HostCallback callback) {
        WasmFunction f;
        f.kind = Kind::Host;
        f.typeIndex = typeIdx;
        f.hostFunc = std::move(callback);
        return f;
    }
};

// =============================================================================
// Import/Export Types
// =============================================================================

struct Import {
    std::string module;
    std::string name;
    DefinitionKind kind;
    uint32_t index;     // Type index for funcs, or local index
};

struct Export {
    std::string name;
    DefinitionKind kind;
    uint32_t index;
};

// =============================================================================
// Element Segment
// =============================================================================

struct ElementSegment {
    bool passive = false;
    bool dropped = false;
    uint32_t tableIndex = 0;
    RefType type;
    std::vector<TableElement> elements;
};

// =============================================================================
// Data Segment
// =============================================================================

struct DataSegment {
    bool passive = false;
    bool dropped = false;
    uint32_t memoryIndex = 0;
    std::vector<uint8_t> data;
};

// =============================================================================
// WasmInstance Class
// =============================================================================

class WasmInstance {
public:
    explicit WasmInstance(std::shared_ptr<WasmModule> module);
    ~WasmInstance() = default;
    
    // No copy
    WasmInstance(const WasmInstance&) = delete;
    WasmInstance& operator=(const WasmInstance&) = delete;
    
    // ==========================================================================
    // Module Access
    // ==========================================================================
    
    WasmModule* module() const { return module_.get(); }
    
    // ==========================================================================
    // Memory Access
    // ==========================================================================
    
    bool hasMemory() const { return !memories_.empty(); }
    uint32_t numMemories() const { return memories_.size(); }
    
    WasmMemory* memory(uint32_t idx = 0) {
        if (idx >= memories_.size()) return nullptr;
        return memories_[idx].get();
    }
    
    const WasmMemory* memory(uint32_t idx = 0) const {
        if (idx >= memories_.size()) return nullptr;
        return memories_[idx].get();
    }
    
    // ==========================================================================
    // Table Access
    // ==========================================================================
    
    bool hasTable() const { return !tables_.empty(); }
    uint32_t numTables() const { return tables_.size(); }
    
    WasmTable* table(uint32_t idx = 0) {
        if (idx >= tables_.size()) return nullptr;
        return tables_[idx].get();
    }
    
    const WasmTable* table(uint32_t idx = 0) const {
        if (idx >= tables_.size()) return nullptr;
        return tables_[idx].get();
    }
    
    // ==========================================================================
    // Global Access
    // ==========================================================================
    
    uint32_t numGlobals() const { return globals_.size(); }
    
    WasmGlobal* global(uint32_t idx) {
        if (idx >= globals_.size()) return nullptr;
        return globals_[idx].get();
    }
    
    const WasmGlobal* global(uint32_t idx) const {
        if (idx >= globals_.size()) return nullptr;
        return globals_[idx].get();
    }
    
    // ==========================================================================
    // Function Access
    // ==========================================================================
    
    uint32_t numFunctions() const { return functions_.size(); }
    
    WasmFunction* function(uint32_t idx) {
        if (idx >= functions_.size()) return nullptr;
        return &functions_[idx];
    }
    
    const WasmFunction* function(uint32_t idx) const {
        if (idx >= functions_.size()) return nullptr;
        return &functions_[idx];
    }
    
    // ==========================================================================
    // Export Access
    // ==========================================================================
    
    const std::vector<Export>& exports() const { return exports_; }
    
    // Find export by name
    const Export* findExport(const std::string& name) const {
        for (const auto& exp : exports_) {
            if (exp.name == name) return &exp;
        }
        return nullptr;
    }
    
    // Get exported function by name
    WasmFunction* getExportedFunction(const std::string& name) {
        auto exp = findExport(name);
        if (!exp || exp->kind != DefinitionKind::Function) return nullptr;
        return function(exp->index);
    }
    
    // Get exported memory by name
    WasmMemory* getExportedMemory(const std::string& name) {
        auto exp = findExport(name);
        if (!exp || exp->kind != DefinitionKind::Memory) return nullptr;
        return memory(exp->index);
    }
    
    // Get exported table by name
    WasmTable* getExportedTable(const std::string& name) {
        auto exp = findExport(name);
        if (!exp || exp->kind != DefinitionKind::Table) return nullptr;
        return table(exp->index);
    }
    
    // Get exported global by name
    WasmGlobal* getExportedGlobal(const std::string& name) {
        auto exp = findExport(name);
        if (!exp || exp->kind != DefinitionKind::Global) return nullptr;
        return global(exp->index);
    }
    
    // ==========================================================================
    // Element/Data Segments
    // ==========================================================================
    
    void dropElemSegment(uint32_t idx) {
        if (idx < elemSegments_.size()) {
            elemSegments_[idx].dropped = true;
        }
    }
    
    void dropDataSegment(uint32_t idx) {
        if (idx < dataSegments_.size()) {
            dataSegments_[idx].dropped = true;
        }
    }
    
    ElementSegment* elemSegment(uint32_t idx) {
        if (idx >= elemSegments_.size()) return nullptr;
        return &elemSegments_[idx];
    }
    
    DataSegment* dataSegment(uint32_t idx) {
        if (idx >= dataSegments_.size()) return nullptr;
        return &dataSegments_[idx];
    }
    
    // ==========================================================================
    // Instantiation
    // ==========================================================================
    
    // Add components
    void addMemory(std::unique_ptr<WasmMemory> mem) {
        memories_.push_back(std::move(mem));
    }
    
    void addTable(std::unique_ptr<WasmTable> table) {
        tables_.push_back(std::move(table));
    }
    
    void addGlobal(std::unique_ptr<WasmGlobal> global) {
        globals_.push_back(std::move(global));
    }
    
    void addFunction(WasmFunction func) {
        functions_.push_back(std::move(func));
    }
    
    void addExport(Export exp) {
        exports_.push_back(std::move(exp));
    }
    
    void addElemSegment(ElementSegment seg) {
        elemSegments_.push_back(std::move(seg));
    }
    
    void addDataSegment(DataSegment seg) {
        dataSegments_.push_back(std::move(seg));
    }
    
    // ==========================================================================
    // Type Access
    // ==========================================================================
    
    const TypeSection* types() const { return types_.get(); }
    void setTypes(std::shared_ptr<TypeSection> types) { types_ = std::move(types); }
    
    const FuncType* funcType(uint32_t idx) const {
        return types_ ? types_->funcType(idx) : nullptr;
    }
    
    // ==========================================================================
    // Start Function
    // ==========================================================================
    
    bool hasStartFunction() const { return startFunction_.has_value(); }
    uint32_t startFunction() const { return startFunction_.value_or(0); }
    void setStartFunction(uint32_t idx) { startFunction_ = idx; }
    
    // ==========================================================================
    // Call Stack (for debugging/traps)
    // ==========================================================================
    
    struct CallFrame {
        uint32_t funcIndex;
        uint32_t offset;        // PC offset in function
        std::string funcName;
    };
    
    void pushFrame(uint32_t funcIdx, uint32_t offset) {
        callStack_.push_back({funcIdx, offset, ""});
    }
    
    void popFrame() {
        if (!callStack_.empty()) callStack_.pop_back();
    }
    
    const std::vector<CallFrame>& callStack() const { return callStack_; }
    
private:
    std::shared_ptr<WasmModule> module_;
    std::shared_ptr<TypeSection> types_;
    
    std::vector<std::unique_ptr<WasmMemory>> memories_;
    std::vector<std::unique_ptr<WasmTable>> tables_;
    std::vector<std::unique_ptr<WasmGlobal>> globals_;
    std::vector<WasmFunction> functions_;
    std::vector<Export> exports_;
    
    std::vector<ElementSegment> elemSegments_;
    std::vector<DataSegment> dataSegments_;
    
    std::optional<uint32_t> startFunction_;
    std::vector<CallFrame> callStack_;
};

// =============================================================================
// Instance Implementation
// =============================================================================

inline WasmInstance::WasmInstance(std::shared_ptr<WasmModule> module)
    : module_(std::move(module)) {
}

} // namespace Zepra::Wasm
