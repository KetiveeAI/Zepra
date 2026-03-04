/**
 * @file WasmModule.h
 * @brief WebAssembly module representation
 * 
 * Represents a parsed and validated WASM module ready for instantiation.
 * 
 * Based on Firefox SpiderMonkey / WebKit JSC
 */

#pragma once

#include "WasmConstants.h"
#include "WasmTypeDef.h"
#include "WasmBinary.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

namespace Zepra::Wasm {

// =============================================================================
// Module Section Metadata
// =============================================================================

struct FunctionDecl {
    uint32_t typeIndex;
    uint32_t codeOffset;
    uint32_t codeSize;
};

struct MemoryDecl {
    uint32_t initial;
    uint32_t maximum;
    bool hasMaximum;
    bool shared;
    bool isMemory64 = false;  // Memory64 proposal
};

struct TableDecl {
    RefType elementType;
    uint32_t initial;
    uint32_t maximum;
    bool hasMaximum;
};

struct GlobalDecl {
    ValType type;
    bool mutable_;
    uint32_t initOffset;    // Offset to init expression in bytecode
};

struct ImportDecl {
    std::string module;
    std::string name;
    DefinitionKind kind;
    uint32_t index;         // Local index after import
    uint32_t typeIndexOrDescriptor;
};

struct ExportDecl {
    std::string name;
    DefinitionKind kind;
    uint32_t index;
};

struct ElemDecl {
    bool passive;
    uint32_t tableIndex;
    RefType type;
    uint32_t initOffset;
    uint32_t count;
};

struct DataDecl {
    bool passive;
    uint32_t memoryIndex;
    uint32_t initOffset;
    uint32_t offset;        // Offset in bytecode for data
    uint32_t size;
};

struct TagDecl {
    uint32_t typeIndex;
};

// =============================================================================
// Custom Section
// =============================================================================

struct CustomSection {
    std::string name;
    std::vector<uint8_t> data;
};

// =============================================================================
// Name Section Data
// =============================================================================

struct NameSection {
    std::optional<std::string> moduleName;
    std::unordered_map<uint32_t, std::string> functionNames;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::string>> localNames;
};

// =============================================================================
// WasmModule Class
// =============================================================================

class WasmModule {
public:
    WasmModule() = default;
    ~WasmModule() = default;
    
    // No copy
    WasmModule(const WasmModule&) = delete;
    WasmModule& operator=(const WasmModule&) = delete;
    
    // Move
    WasmModule(WasmModule&&) = default;
    WasmModule& operator=(WasmModule&&) = default;
    
    // ==========================================================================
    // Static factory
    // ==========================================================================
    
    static std::unique_ptr<WasmModule> parse(const uint8_t* bytes, size_t length, 
                                              std::string* errorOut = nullptr);
    
    // ==========================================================================
    // Bytecode Access
    // ==========================================================================
    
    const uint8_t* bytecode() const { return bytecode_.data(); }
    size_t bytecodeSize() const { return bytecode_.size(); }
    
    // ==========================================================================
    // Type Section
    // ==========================================================================
    
    const TypeSection& types() const { return types_; }
    TypeSection& types() { return types_; }
    
    uint32_t numTypes() const { return types_.size(); }
    const FuncType* funcType(uint32_t idx) const { return types_.funcType(idx); }
    
    // ==========================================================================
    // Functions
    // ==========================================================================
    
    const std::vector<FunctionDecl>& functions() const { return functions_; }
    uint32_t numFunctions() const { return functions_.size(); }
    uint32_t numImportedFunctions() const { return numImportedFuncs_; }
    
    void addFunction(FunctionDecl decl) { functions_.push_back(decl); }
    
    // ==========================================================================
    // Memories
    // ==========================================================================
    
    const std::vector<MemoryDecl>& memories() const { return memories_; }
    uint32_t numMemories() const { return memories_.size(); }
    
    void addMemory(MemoryDecl decl) { memories_.push_back(decl); }
    
    // ==========================================================================
    // Tables
    // ==========================================================================
    
    const std::vector<TableDecl>& tables() const { return tables_; }
    uint32_t numTables() const { return tables_.size(); }
    
    void addTable(TableDecl decl) { tables_.push_back(decl); }
    
    // ==========================================================================
    // Globals
    // ==========================================================================
    
    const std::vector<GlobalDecl>& globals() const { return globals_; }
    uint32_t numGlobals() const { return globals_.size(); }
    
    void addGlobal(GlobalDecl decl) { globals_.push_back(decl); }
    
    // ==========================================================================
    // Imports
    // ==========================================================================
    
    const std::vector<ImportDecl>& imports() const { return imports_; }
    uint32_t numImports() const { return imports_.size(); }
    
    void addImport(ImportDecl decl) {
        imports_.push_back(decl);
        switch (decl.kind) {
            case DefinitionKind::Function: numImportedFuncs_++; break;
            case DefinitionKind::Memory: numImportedMems_++; break;
            case DefinitionKind::Table: numImportedTables_++; break;
            case DefinitionKind::Global: numImportedGlobals_++; break;
            case DefinitionKind::Tag: numImportedTags_++; break;
        }
    }
    
    // ==========================================================================
    // Exports
    // ==========================================================================
    
    const std::vector<ExportDecl>& exports() const { return exports_; }
    uint32_t numExports() const { return exports_.size(); }
    
    void addExport(ExportDecl decl) { exports_.push_back(decl); }
    
    // ==========================================================================
    // Elements
    // ==========================================================================
    
    const std::vector<ElemDecl>& elements() const { return elements_; }
    uint32_t numElements() const { return elements_.size(); }
    
    void addElement(ElemDecl decl) { elements_.push_back(decl); }
    
    // ==========================================================================
    // Data
    // ==========================================================================
    
    const std::vector<DataDecl>& dataSegments() const { return dataSegments_; }
    uint32_t numDataSegments() const { return dataSegments_.size(); }
    
    void addDataSegment(DataDecl decl) { dataSegments_.push_back(decl); }
    
    // ==========================================================================
    // Tags
    // ==========================================================================
    
    const std::vector<TagDecl>& tags() const { return tags_; }
    uint32_t numTags() const { return tags_.size(); }
    
    void addTag(TagDecl decl) { tags_.push_back(decl); }
    
    // ==========================================================================
    // Start Function
    // ==========================================================================
    
    bool hasStartFunction() const { return startFunction_.has_value(); }
    uint32_t startFunction() const { return startFunction_.value_or(0); }
    void setStartFunction(uint32_t idx) { startFunction_ = idx; }
    
    // ==========================================================================
    // Custom Sections
    // ==========================================================================
    
    const std::vector<CustomSection>& customSections() const { return customSections_; }
    void addCustomSection(CustomSection sec) { customSections_.push_back(std::move(sec)); }
    
    // ==========================================================================
    // Name Section
    // ==========================================================================
    
    const NameSection& names() const { return names_; }
    NameSection& names() { return names_; }
    
    std::string functionName(uint32_t idx) const {
        auto it = names_.functionNames.find(idx);
        return it != names_.functionNames.end() ? it->second : "";
    }
    
    // ==========================================================================
    // Code Section
    // ==========================================================================
    
    ByteRange codeSection() const { return codeSection_; }
    void setCodeSection(ByteRange range) { codeSection_ = range; }
    
    // ==========================================================================
    // Bytecode storage
    // ==========================================================================
    
    void setBytecode(std::vector<uint8_t> bc) { bytecode_ = std::move(bc); }
    
private:
    std::vector<uint8_t> bytecode_;
    TypeSection types_;
    
    std::vector<FunctionDecl> functions_;
    std::vector<MemoryDecl> memories_;
    std::vector<TableDecl> tables_;
    std::vector<GlobalDecl> globals_;
    std::vector<ImportDecl> imports_;
    std::vector<ExportDecl> exports_;
    std::vector<ElemDecl> elements_;
    std::vector<DataDecl> dataSegments_;
    std::vector<TagDecl> tags_;
    std::vector<CustomSection> customSections_;
    
    NameSection names_;
    ByteRange codeSection_;
    std::optional<uint32_t> startFunction_;
    
    uint32_t numImportedFuncs_ = 0;
    uint32_t numImportedMems_ = 0;
    uint32_t numImportedTables_ = 0;
    uint32_t numImportedGlobals_ = 0;
    uint32_t numImportedTags_ = 0;
};

} // namespace Zepra::Wasm
