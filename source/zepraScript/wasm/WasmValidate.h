/**
 * @file WasmValidate.h
 * @brief WebAssembly module validation
 * 
 * Provides comprehensive validation for WASM modules including:
 * - Type checking
 * - Control flow validation
 * - Memory/table access validation
 * - Import/export validation
 * 
 * Based on Firefox SpiderMonkey WasmValidate.h
 */

#pragma once

#include "WasmConstants.h"
#include "WasmValType.h"
#include "WasmTypeDef.h"
#include "WasmBinary.h"
#include <vector>
#include <stack>
#include <string>
#include <optional>
#include <set>

namespace Zepra::Wasm {

// =============================================================================
// Validation Error
// =============================================================================

struct ValidationError {
    std::string message;
    size_t offset;
    
    ValidationError() : offset(0) {}
    ValidationError(const std::string& msg, size_t off = 0)
        : message(msg), offset(off) {}
};

// =============================================================================
// Control Frame (for control flow validation)
// =============================================================================

struct ControlFrame {
    enum class Kind : uint8_t {
        Block,
        Loop,
        If,
        Else,
        Try,
        Catch,
        CatchAll
    };
    
    Kind kind;
    BlockType blockType;
    size_t height;           // Value stack height at start
    bool unreachable;        // Stack polymorphic after unreachable
    uint32_t labelIdx;       // For br targets
    
    ControlFrame(Kind k, BlockType bt, size_t h)
        : kind(k), blockType(bt), height(h), unreachable(false), labelIdx(0) {}
    
    bool isLoop() const { return kind == Kind::Loop; }
    bool isIf() const { return kind == Kind::If; }
    bool isTry() const { return kind == Kind::Try; }
};

// =============================================================================
// Value Stack Entry
// =============================================================================

struct StackEntry {
    ValType type;
    bool unknown;  // For stack polymorphism
    
    StackEntry() : type(ValType::i32()), unknown(false) {}
    explicit StackEntry(ValType t) : type(t), unknown(false) {}
    static StackEntry unknownEntry() {
        StackEntry e;
        e.unknown = true;
        return e;
    }
};

// =============================================================================
// Validation Context
// =============================================================================

class ValidationContext {
public:
    ValidationContext() = default;
    
    // ==========================================================================
    // Type management
    // ==========================================================================
    
    void setTypes(const TypeSection* types) { types_ = types; }
    const TypeSection* types() const { return types_; }
    
    const FuncType* funcType(uint32_t idx) const {
        if (!types_) return nullptr;
        return types_->funcType(idx);
    }
    
    // ==========================================================================
    // Function management
    // ==========================================================================
    
    void addFunction(uint32_t typeIdx) {
        funcTypes_.push_back(typeIdx);
    }
    
    uint32_t numFunctions() const { return funcTypes_.size(); }
    
    const FuncType* functionType(uint32_t idx) const {
        if (idx >= funcTypes_.size()) return nullptr;
        return funcType(funcTypes_[idx]);
    }
    
    // ==========================================================================
    // Memory management
    // ==========================================================================
    
    void addMemory(bool shared = false) {
        memories_.push_back({shared});
    }
    
    bool hasMemory() const { return !memories_.empty(); }
    uint32_t numMemories() const { return memories_.size(); }
    
    // ==========================================================================
    // Table management
    // ==========================================================================
    
    void addTable(RefType elemType) {
        tables_.push_back({elemType});
    }
    
    bool hasTable() const { return !tables_.empty(); }
    uint32_t numTables() const { return tables_.size(); }
    
    RefType tableType(uint32_t idx) const {
        if (idx >= tables_.size()) return RefType::funcRef();
        return tables_[idx].elemType;
    }
    
    // ==========================================================================
    // Global management
    // ==========================================================================
    
    void addGlobal(ValType type, bool mutable_) {
        globals_.push_back({type, mutable_});
    }
    
    uint32_t numGlobals() const { return globals_.size(); }
    
    ValType globalType(uint32_t idx) const {
        if (idx >= globals_.size()) return ValType::i32();
        return globals_[idx].type;
    }
    
    bool globalMutable(uint32_t idx) const {
        if (idx >= globals_.size()) return false;
        return globals_[idx].mutable_;
    }
    
    // ==========================================================================
    // Tag management (for exceptions)
    // ==========================================================================
    
    void addTag(uint32_t typeIdx) {
        tags_.push_back(typeIdx);
    }
    
    uint32_t numTags() const { return tags_.size(); }
    
    // ==========================================================================
    // Data segments
    // ==========================================================================
    
    void addDataSegment() { numDataSegments_++; }
    uint32_t numDataSegments() const { return numDataSegments_; }
    
    // ==========================================================================
    // Element segments
    // ==========================================================================
    
    void addElementSegment() { numElemSegments_++; }
    uint32_t numElementSegments() const { return numElemSegments_; }
    
private:
    const TypeSection* types_ = nullptr;
    std::vector<uint32_t> funcTypes_;
    
    struct MemoryInfo { bool shared; };
    std::vector<MemoryInfo> memories_;
    
    struct TableInfo { RefType elemType; };
    std::vector<TableInfo> tables_;
    
    struct GlobalInfo { ValType type; bool mutable_; };
    std::vector<GlobalInfo> globals_;
    
    std::vector<uint32_t> tags_;
    uint32_t numDataSegments_ = 0;
    uint32_t numElemSegments_ = 0;
};

// =============================================================================
// Function Validator
// =============================================================================

class FunctionValidator {
public:
    explicit FunctionValidator(const ValidationContext* ctx, const FuncType* funcType)
        : ctx_(ctx)
        , funcType_(funcType) {
        // Initialize locals from parameters
        for (const auto& param : funcType->params()) {
            locals_.push_back(param);
        }
    }
    
    // ==========================================================================
    // Local management
    // ==========================================================================
    
    bool addLocal(ValType type, uint32_t count = 1) {
        for (uint32_t i = 0; i < count; i++) {
            locals_.push_back(type);
        }
        return true;
    }
    
    uint32_t numLocals() const { return locals_.size(); }
    
    std::optional<ValType> localType(uint32_t idx) const {
        if (idx >= locals_.size()) return std::nullopt;
        return locals_[idx];
    }
    
    // ==========================================================================
    // Value stack operations
    // ==========================================================================
    
    void push(ValType type) {
        valueStack_.push_back(StackEntry(type));
    }
    
    void pushUnknown() {
        valueStack_.push_back(StackEntry::unknownEntry());
    }
    
    std::optional<ValType> pop() {
        if (valueStack_.empty()) {
            if (!controlStack_.empty() && controlStack_.back().unreachable) {
                return ValType::i32();  // Return any type when polymorphic
            }
            return std::nullopt;
        }
        auto entry = valueStack_.back();
        valueStack_.pop_back();
        return entry.type;
    }
    
    bool popExpecting(ValType expected) {
        auto actual = pop();
        if (!actual) return false;
        if (isPolymorphic()) return true;
        return actual->isSubtypeOf(expected);
    }
    
    size_t stackHeight() const { return valueStack_.size(); }
    
    bool isPolymorphic() const {
        return !controlStack_.empty() && controlStack_.back().unreachable;
    }
    
    // ==========================================================================
    // Control flow operations
    // ==========================================================================
    
    bool pushControl(ControlFrame::Kind kind, BlockType blockType) {
        controlStack_.emplace_back(kind, blockType, stackHeight());
        return true;
    }
    
    std::optional<ControlFrame> popControl() {
        if (controlStack_.empty()) return std::nullopt;
        auto frame = controlStack_.back();
        controlStack_.pop_back();
        return frame;
    }
    
    ControlFrame* currentControl() {
        if (controlStack_.empty()) return nullptr;
        return &controlStack_.back();
    }
    
    std::optional<ControlFrame*> labelTarget(uint32_t depth) {
        if (depth >= controlStack_.size()) return std::nullopt;
        return &controlStack_[controlStack_.size() - 1 - depth];
    }
    
    void setUnreachable() {
        if (!controlStack_.empty()) {
            controlStack_.back().unreachable = true;
            // Reset stack to frame height
            while (valueStack_.size() > controlStack_.back().height) {
                valueStack_.pop_back();
            }
        }
    }
    
    size_t controlDepth() const { return controlStack_.size(); }
    
    // ==========================================================================
    // Validation state
    // ==========================================================================
    
    bool hasError() const { return !error_.message.empty(); }
    const ValidationError& error() const { return error_; }
    
    void setError(const std::string& msg, size_t offset = 0) {
        error_ = ValidationError(msg, offset);
    }
    
    // ==========================================================================
    // Complete function validation
    // ==========================================================================
    
    bool finalize() {
        if (controlStack_.size() != 0) {
            setError("unbalanced control flow");
            return false;
        }
        
        // Check return values match
        for (const auto& result : funcType_->results()) {
            if (!popExpecting(result)) {
                setError("type mismatch in function result");
                return false;
            }
        }
        
        if (!valueStack_.empty()) {
            setError("values left on stack after function");
            return false;
        }
        
        return true;
    }
    
private:
    const ValidationContext* ctx_;
    const FuncType* funcType_;
    std::vector<ValType> locals_;
    std::vector<StackEntry> valueStack_;
    std::vector<ControlFrame> controlStack_;
    ValidationError error_;
};

// =============================================================================
// Module Validator
// =============================================================================

class ModuleValidator {
public:
    ModuleValidator() = default;
    
    // ==========================================================================
    // Section validation
    // ==========================================================================
    
    bool validateTypeSection(Decoder& decoder, uint32_t count);
    bool validateImportSection(Decoder& decoder, uint32_t count);
    bool validateFunctionSection(Decoder& decoder, uint32_t count);
    bool validateTableSection(Decoder& decoder, uint32_t count);
    bool validateMemorySection(Decoder& decoder, uint32_t count);
    bool validateGlobalSection(Decoder& decoder, uint32_t count);
    bool validateExportSection(Decoder& decoder, uint32_t count);
    bool validateStartSection(Decoder& decoder);
    bool validateElementSection(Decoder& decoder, uint32_t count);
    bool validateCodeSection(Decoder& decoder, uint32_t count);
    bool validateDataSection(Decoder& decoder, uint32_t count);
    bool validateDataCountSection(Decoder& decoder);
    bool validateTagSection(Decoder& decoder, uint32_t count);
    
    // ==========================================================================
    // Function body validation
    // ==========================================================================
    
    bool validateFunctionBody(Decoder& decoder, uint32_t funcIdx);
    
    // ==========================================================================
    // Expression validation
    // ==========================================================================
    
    bool validateConstExpr(Decoder& decoder, ValType expectedType);
    
    // ==========================================================================
    // Complete module validation
    // ==========================================================================
    
    bool validate(const uint8_t* bytes, size_t length);
    bool validate(const std::vector<uint8_t>& bytes) {
        return validate(bytes.data(), bytes.size());
    }
    
    // ==========================================================================
    // Error handling
    // ==========================================================================
    
    bool hasError() const { return !error_.message.empty(); }
    const ValidationError& error() const { return error_; }
    
    ValidationContext& context() { return ctx_; }
    const ValidationContext& context() const { return ctx_; }
    
private:
    bool setError(const std::string& msg, size_t offset = 0) {
        error_ = ValidationError(msg, offset);
        return false;
    }
    
    // Instruction validation helper
    bool validateInstruction(Decoder& decoder, FunctionValidator& validator, Opcode op);
    
    ValidationContext ctx_;
    TypeSection types_;
    ValidationError error_;
    
    // Track expected function count
    uint32_t numFunctionDecls_ = 0;
    uint32_t numFunctionBodies_ = 0;
    std::optional<uint32_t> dataCount_;
    std::set<std::string> exportNames_;
};

// =============================================================================
// Quick validation function
// =============================================================================

inline bool validateModule(const uint8_t* bytes, size_t length, std::string* errorOut = nullptr) {
    ModuleValidator validator;
    bool valid = validator.validate(bytes, length);
    if (!valid && errorOut) {
        *errorOut = validator.error().message;
    }
    return valid;
}

inline bool validateModule(const std::vector<uint8_t>& bytes, std::string* errorOut = nullptr) {
    return validateModule(bytes.data(), bytes.size(), errorOut);
}

} // namespace Zepra::Wasm
