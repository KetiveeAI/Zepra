/**
 * @file WasmValidate.cpp
 * @brief WebAssembly module validation implementation
 */

#include "wasm/WasmValidate.h"
#include "wasm/WasmBinary.h"
#include "wasm/WasmMemory.h"
#include <algorithm>

namespace Zepra::Wasm {

// =============================================================================
// Type Section Validation
// =============================================================================

bool ModuleValidator::validateTypeSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t form;
        if (!decoder.readFixedU8(&form)) {
            return setError("expected type form", decoder.currentOffset());
        }
        
        if (form == static_cast<uint8_t>(TypeCode::Func)) {
            // Read parameters
            uint32_t numParams;
            if (!decoder.readVarU32(&numParams)) {
                return setError("expected param count", decoder.currentOffset());
            }
            
            std::vector<ValType> params;
            for (uint32_t j = 0; j < numParams; j++) {
                ValType type;
                if (!decoder.readValType(&type)) {
                    return setError("invalid param type", decoder.currentOffset());
                }
                params.push_back(type);
            }
            
            // Read results
            uint32_t numResults;
            if (!decoder.readVarU32(&numResults)) {
                return setError("expected result count", decoder.currentOffset());
            }
            
            std::vector<ValType> results;
            for (uint32_t j = 0; j < numResults; j++) {
                ValType type;
                if (!decoder.readValType(&type)) {
                    return setError("invalid result type", decoder.currentOffset());
                }
                results.push_back(type);
            }
            
            types_.addType(TypeDef::func(FuncType(std::move(params), std::move(results))));
        } else {
            return setError("unknown type form", decoder.currentOffset());
        }
    }
    
    ctx_.setTypes(&types_);
    return true;
}

// =============================================================================
// Import Section Validation
// =============================================================================

bool ModuleValidator::validateImportSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        std::string moduleName, fieldName;
        if (!decoder.readString(&moduleName) || !decoder.readString(&fieldName)) {
            return setError("invalid import names", decoder.currentOffset());
        }
        
        uint8_t kind;
        if (!decoder.readFixedU8(&kind)) {
            return setError("expected import kind", decoder.currentOffset());
        }
        
        switch (static_cast<DefinitionKind>(kind)) {
            case DefinitionKind::Function: {
                uint32_t typeIdx;
                if (!decoder.readVarU32(&typeIdx)) {
                    return setError("expected function type index", decoder.currentOffset());
                }
                if (typeIdx >= types_.size()) {
                    return setError("type index out of bounds", decoder.currentOffset());
                }
                ctx_.addFunction(typeIdx);
                break;
            }
            
            case DefinitionKind::Table: {
                uint8_t elemType;
                if (!decoder.readFixedU8(&elemType)) {
                    return setError("expected table element type", decoder.currentOffset());
                }
                uint32_t flags, initial, maximum = 0;
                if (!decoder.readVarU32(&flags) || !decoder.readVarU32(&initial)) {
                    return setError("invalid table limits", decoder.currentOffset());
                }
                if (flags & static_cast<uint8_t>(LimitsFlags::HasMaximum)) {
                    if (!decoder.readVarU32(&maximum)) {
                        return setError("invalid table maximum", decoder.currentOffset());
                    }
                }
                ctx_.addTable(RefType::funcRef());
                break;
            }
            
            case DefinitionKind::Memory: {
                uint32_t flags, initial, maximum = 0;
                if (!decoder.readVarU32(&flags) || !decoder.readVarU32(&initial)) {
                    return setError("invalid memory limits", decoder.currentOffset());
                }
                if (flags & static_cast<uint8_t>(LimitsFlags::HasMaximum)) {
                    if (!decoder.readVarU32(&maximum)) {
                        return setError("invalid memory maximum", decoder.currentOffset());
                    }
                }
                bool shared = flags & static_cast<uint8_t>(LimitsFlags::IsShared);
                ctx_.addMemory(shared);
                break;
            }
            
            case DefinitionKind::Global: {
                ValType type;
                if (!decoder.readValType(&type)) {
                    return setError("invalid global type", decoder.currentOffset());
                }
                uint8_t mutability;
                if (!decoder.readFixedU8(&mutability)) {
                    return setError("expected global mutability", decoder.currentOffset());
                }
                ctx_.addGlobal(type, mutability != 0);
                break;
            }
            
            case DefinitionKind::Tag: {
                uint8_t attribute;
                if (!decoder.readFixedU8(&attribute)) {
                    return setError("expected tag attribute", decoder.currentOffset());
                }
                uint32_t typeIdx;
                if (!decoder.readVarU32(&typeIdx)) {
                    return setError("expected tag type index", decoder.currentOffset());
                }
                ctx_.addTag(typeIdx);
                break;
            }
            
            default:
                return setError("unknown import kind", decoder.currentOffset());
        }
    }
    return true;
}

// =============================================================================
// Function Section Validation
// =============================================================================

bool ModuleValidator::validateFunctionSection(Decoder& decoder, uint32_t count) {
    numFunctionDecls_ = count;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t typeIdx;
        if (!decoder.readVarU32(&typeIdx)) {
            return setError("expected function type index", decoder.currentOffset());
        }
        if (typeIdx >= types_.size()) {
            return setError("type index out of bounds", decoder.currentOffset());
        }
        ctx_.addFunction(typeIdx);
    }
    return true;
}

// =============================================================================
// Table Section Validation
// =============================================================================

bool ModuleValidator::validateTableSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t elemType;
        if (!decoder.readFixedU8(&elemType)) {
            return setError("expected table element type", decoder.currentOffset());
        }
        
        uint32_t flags, initial, maximum = 0;
        if (!decoder.readVarU32(&flags) || !decoder.readVarU32(&initial)) {
            return setError("invalid table limits", decoder.currentOffset());
        }
        if (flags & static_cast<uint8_t>(LimitsFlags::HasMaximum)) {
            if (!decoder.readVarU32(&maximum)) {
                return setError("invalid table maximum", decoder.currentOffset());
            }
            if (maximum < initial) {
                return setError("table maximum less than initial", decoder.currentOffset());
            }
        }
        
        ctx_.addTable(RefType::funcRef());
    }
    return true;
}

// =============================================================================
// Memory Section Validation
// =============================================================================

bool ModuleValidator::validateMemorySection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t flags, initial, maximum = 0;
        if (!decoder.readVarU32(&flags) || !decoder.readVarU32(&initial)) {
            return setError("invalid memory limits", decoder.currentOffset());
        }
        
        if (initial > MaxPages32) {
            return setError("memory initial exceeds maximum", decoder.currentOffset());
        }
        
        if (flags & static_cast<uint8_t>(LimitsFlags::HasMaximum)) {
            if (!decoder.readVarU32(&maximum)) {
                return setError("invalid memory maximum", decoder.currentOffset());
            }
            if (maximum > MaxPages32) {
                return setError("memory maximum exceeds limit", decoder.currentOffset());
            }
            if (maximum < initial) {
                return setError("memory maximum less than initial", decoder.currentOffset());
            }
        }
        
        bool shared = flags & static_cast<uint8_t>(LimitsFlags::IsShared);
        if (shared && !(flags & static_cast<uint8_t>(LimitsFlags::HasMaximum))) {
            return setError("shared memory requires maximum", decoder.currentOffset());
        }
        
        ctx_.addMemory(shared);
    }
    return true;
}

// =============================================================================
// Global Section Validation
// =============================================================================

bool ModuleValidator::validateGlobalSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ValType type;
        if (!decoder.readValType(&type)) {
            return setError("invalid global type", decoder.currentOffset());
        }
        
        uint8_t mutability;
        if (!decoder.readFixedU8(&mutability)) {
            return setError("expected global mutability", decoder.currentOffset());
        }
        
        // Validate init expression
        if (!validateConstExpr(decoder, type)) {
            return false;
        }
        
        ctx_.addGlobal(type, mutability != 0);
    }
    return true;
}

// =============================================================================
// Export Section Validation
// =============================================================================

bool ModuleValidator::validateExportSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        std::string name;
        if (!decoder.readString(&name)) {
            return setError("invalid export name", decoder.currentOffset());
        }
        
        // Check for duplicate export names
        if (exportNames_.count(name)) {
            return setError("duplicate export name: " + name, decoder.currentOffset());
        }
        exportNames_.insert(name);
        
        uint8_t kind;
        if (!decoder.readFixedU8(&kind)) {
            return setError("expected export kind", decoder.currentOffset());
        }
        
        uint32_t index;
        if (!decoder.readVarU32(&index)) {
            return setError("expected export index", decoder.currentOffset());
        }
        
        switch (static_cast<DefinitionKind>(kind)) {
            case DefinitionKind::Function:
                if (index >= ctx_.numFunctions()) {
                    return setError("export function index out of bounds", decoder.currentOffset());
                }
                break;
            case DefinitionKind::Table:
                if (index >= ctx_.numTables()) {
                    return setError("export table index out of bounds", decoder.currentOffset());
                }
                break;
            case DefinitionKind::Memory:
                if (index >= ctx_.numMemories()) {
                    return setError("export memory index out of bounds", decoder.currentOffset());
                }
                break;
            case DefinitionKind::Global:
                if (index >= ctx_.numGlobals()) {
                    return setError("export global index out of bounds", decoder.currentOffset());
                }
                break;
            case DefinitionKind::Tag:
                if (index >= ctx_.numTags()) {
                    return setError("export tag index out of bounds", decoder.currentOffset());
                }
                break;
            default:
                return setError("unknown export kind", decoder.currentOffset());
        }
    }
    return true;
}

// =============================================================================
// Start Section Validation
// =============================================================================

bool ModuleValidator::validateStartSection(Decoder& decoder) {
    uint32_t funcIdx;
    if (!decoder.readVarU32(&funcIdx)) {
        return setError("expected start function index", decoder.currentOffset());
    }
    
    if (funcIdx >= ctx_.numFunctions()) {
        return setError("start function index out of bounds", decoder.currentOffset());
    }
    
    const FuncType* type = ctx_.functionType(funcIdx);
    if (!type || !type->params().empty() || !type->results().empty()) {
        return setError("start function must have no params and no results", decoder.currentOffset());
    }
    
    return true;
}

// =============================================================================
// Element Section Validation
// =============================================================================

bool ModuleValidator::validateElementSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t flags;
        if (!decoder.readVarU32(&flags)) {
            return setError("expected element flags", decoder.currentOffset());
        }
        
        auto kind = static_cast<ElemSegmentKind>(flags & 0x3);
        
        // Handle table index for active segments
        if (kind == ElemSegmentKind::ActiveWithTableIndex) {
            uint32_t tableIdx;
            if (!decoder.readVarU32(&tableIdx)) {
                return setError("expected table index", decoder.currentOffset());
            }
            if (tableIdx >= ctx_.numTables()) {
                return setError("table index out of bounds", decoder.currentOffset());
            }
        }
        
        // Handle offset expression for active segments
        if (kind == ElemSegmentKind::Active || kind == ElemSegmentKind::ActiveWithTableIndex) {
            if (!validateConstExpr(decoder, ValType::i32())) {
                return false;
            }
        }
        
        // Read element count
        uint32_t elemCount;
        if (!decoder.readVarU32(&elemCount)) {
            return setError("expected element count", decoder.currentOffset());
        }
        
        // Skip elements for now (simplified)
        bool isExprs = (flags & static_cast<uint32_t>(ElemSegmentPayload::Expressions)) != 0;
        for (uint32_t j = 0; j < elemCount; j++) {
            if (isExprs) {
                // Skip expression
                Opcode op;
                while (decoder.readOp(&op) && op.asOp() != Op::End) {
                    // Skip expression content
                }
            } else {
                uint32_t funcIdx;
                if (!decoder.readVarU32(&funcIdx)) {
                    return setError("expected function index", decoder.currentOffset());
                }
            }
        }
        
        ctx_.addElementSegment();
    }
    return true;
}

// =============================================================================
// Code Section Validation
// =============================================================================

bool ModuleValidator::validateCodeSection(Decoder& decoder, uint32_t count) {
    if (count != numFunctionDecls_) {
        return setError("code section count mismatch", decoder.currentOffset());
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t bodySize;
        if (!decoder.readVarU32(&bodySize)) {
            return setError("expected function body size", decoder.currentOffset());
        }
        
        const uint8_t* bodyStart = decoder.currentPosition();
        
        if (!validateFunctionBody(decoder, numFunctionBodies_)) {
            return false;
        }
        
        numFunctionBodies_++;
        
        // Ensure we consumed exactly bodySize bytes
        size_t consumed = decoder.currentPosition() - bodyStart;
        if (consumed != bodySize) {
            return setError("function body size mismatch", decoder.currentOffset());
        }
    }
    return true;
}

// =============================================================================
// Function Body Validation
// =============================================================================

bool ModuleValidator::validateFunctionBody(Decoder& decoder, uint32_t funcIdx) {
    const FuncType* funcType = ctx_.functionType(funcIdx);
    if (!funcType) {
        return setError("function type not found", decoder.currentOffset());
    }
    
    FunctionValidator validator(&ctx_, funcType);
    
    // Read locals
    uint32_t numLocalDecls;
    if (!decoder.readVarU32(&numLocalDecls)) {
        return setError("expected local count", decoder.currentOffset());
    }
    
    for (uint32_t i = 0; i < numLocalDecls; i++) {
        uint32_t count;
        ValType type;
        if (!decoder.readVarU32(&count) || !decoder.readValType(&type)) {
            return setError("invalid local declaration", decoder.currentOffset());
        }
        validator.addLocal(type, count);
    }
    
    // Push implicit function block
    std::vector<ValType> results = funcType->results();
    BlockType blockType = results.empty() ? BlockType::void_() 
                        : results.size() == 1 ? BlockType::single(results[0])
                        : BlockType::typeIndex(0);  // Multi-value needs type index
    validator.pushControl(ControlFrame::Kind::Block, blockType);
    
    // Validate instructions
    Opcode op;
    while (decoder.readOp(&op)) {
        if (op.isOp() && op.asOp() == Op::End) {
            auto frame = validator.popControl();
            if (!frame) {
                return setError("unmatched end", decoder.currentOffset());
            }
            if (validator.controlDepth() == 0) {
                break;  // Function end
            }
            continue;
        }
        
        // Simplified opcode validation - full implementation would validate each opcode
        if (!validateInstruction(decoder, validator, op)) {
            error_ = validator.error();
            return false;
        }
    }
    
    if (!validator.finalize()) {
        error_ = validator.error();
        return false;
    }
    
    return true;
}

// Helper for instruction validation (simplified)
bool ModuleValidator::validateInstruction(Decoder& decoder, FunctionValidator& validator, Opcode op) {
    // This is a simplified implementation - a full implementation would validate 
    // each opcode's operands and stack effects
    
    if (!op.isOp()) {
        // Handle prefixed opcodes
        return true;  // Simplified: skip for now
    }
    
    switch (op.asOp()) {
        case Op::Unreachable:
            validator.setUnreachable();
            break;
            
        case Op::Nop:
            break;
            
        case Op::Block:
        case Op::Loop:
        case Op::If: {
            BlockType bt;
            if (!decoder.readBlockType(&bt)) {
                validator.setError("invalid block type", decoder.currentOffset());
                return false;
            }
            auto kind = op.asOp() == Op::Loop ? ControlFrame::Kind::Loop 
                      : op.asOp() == Op::If ? ControlFrame::Kind::If 
                      : ControlFrame::Kind::Block;
            if (op.asOp() == Op::If) {
                if (!validator.popExpecting(ValType::i32())) {
                    validator.setError("if requires i32 condition", decoder.currentOffset());
                    return false;
                }
            }
            validator.pushControl(kind, bt);
            break;
        }
        
        case Op::Else: {
            auto* frame = validator.currentControl();
            if (!frame || frame->kind != ControlFrame::Kind::If) {
                validator.setError("else without if", decoder.currentOffset());
                return false;
            }
            frame->kind = ControlFrame::Kind::Else;
            break;
        }
        
        case Op::Br:
        case Op::BrIf: {
            uint32_t depth;
            if (!decoder.readVarU32(&depth)) {
                validator.setError("expected branch depth", decoder.currentOffset());
                return false;
            }
            if (op.asOp() == Op::BrIf) {
                if (!validator.popExpecting(ValType::i32())) {
                    validator.setError("br_if requires i32 condition", decoder.currentOffset());
                    return false;
                }
            }
            if (op.asOp() == Op::Br) {
                validator.setUnreachable();
            }
            break;
        }
        
        case Op::Return:
            validator.setUnreachable();
            break;
            
        case Op::Call: {
            uint32_t funcIdx;
            if (!decoder.readVarU32(&funcIdx)) {
                validator.setError("expected function index", decoder.currentOffset());
                return false;
            }
            break;
        }
        
        case Op::Drop:
            validator.pop();
            break;
            
        case Op::LocalGet:
        case Op::LocalSet:
        case Op::LocalTee: {
            uint32_t localIdx;
            if (!decoder.readVarU32(&localIdx)) {
                validator.setError("expected local index", decoder.currentOffset());
                return false;
            }
            auto type = validator.localType(localIdx);
            if (!type) {
                validator.setError("local index out of bounds", decoder.currentOffset());
                return false;
            }
            if (op.asOp() == Op::LocalGet || op.asOp() == Op::LocalTee) {
                validator.push(*type);
            }
            if (op.asOp() == Op::LocalSet || op.asOp() == Op::LocalTee) {
                validator.pop();
            }
            break;
        }
        
        case Op::GlobalGet:
        case Op::GlobalSet: {
            uint32_t globalIdx;
            if (!decoder.readVarU32(&globalIdx)) {
                validator.setError("expected global index", decoder.currentOffset());
                return false;
            }
            break;
        }
        
        case Op::I32Const: {
            int32_t value;
            if (!decoder.readVarS32(&value)) {
                validator.setError("expected i32 constant", decoder.currentOffset());
                return false;
            }
            validator.push(ValType::i32());
            break;
        }
        
        case Op::I64Const: {
            int64_t value;
            if (!decoder.readVarS64(&value)) {
                validator.setError("expected i64 constant", decoder.currentOffset());
                return false;
            }
            validator.push(ValType::i64());
            break;
        }
        
        case Op::F32Const: {
            float value;
            if (!decoder.readFixedF32(&value)) {
                validator.setError("expected f32 constant", decoder.currentOffset());
                return false;
            }
            validator.push(ValType::f32());
            break;
        }
        
        case Op::F64Const: {
            double value;
            if (!decoder.readFixedF64(&value)) {
                validator.setError("expected f64 constant", decoder.currentOffset());
                return false;
            }
            validator.push(ValType::f64());
            break;
        }
        
        // Memory operations
        case Op::I32Load:
        case Op::I64Load:
        case Op::F32Load:
        case Op::F64Load:
        case Op::I32Load8S:
        case Op::I32Load8U:
        case Op::I32Load16S:
        case Op::I32Load16U:
        case Op::I64Load8S:
        case Op::I64Load8U:
        case Op::I64Load16S:
        case Op::I64Load16U:
        case Op::I64Load32S:
        case Op::I64Load32U:
        case Op::I32Store:
        case Op::I64Store:
        case Op::F32Store:
        case Op::F64Store:
        case Op::I32Store8:
        case Op::I32Store16:
        case Op::I64Store8:
        case Op::I64Store16:
        case Op::I64Store32: {
            uint32_t align, offset;
            if (!decoder.readVarU32(&align) || !decoder.readVarU32(&offset)) {
                validator.setError("expected memory operands", decoder.currentOffset());
                return false;
            }
            if (!ctx_.hasMemory()) {
                validator.setError("memory operation without memory", decoder.currentOffset());
                return false;
            }
            break;
        }
        
        case Op::MemorySize:
        case Op::MemoryGrow: {
            uint8_t memIdx;
            if (!decoder.readFixedU8(&memIdx)) {
                validator.setError("expected memory index", decoder.currentOffset());
                return false;
            }
            break;
        }
        
        // Numeric operations (simplified - just track stack)
        case Op::I32Eqz:
        case Op::I32Clz:
        case Op::I32Ctz:
        case Op::I32Popcnt:
            validator.pop();
            validator.push(ValType::i32());
            break;
            
        case Op::I32Add:
        case Op::I32Sub:
        case Op::I32Mul:
        case Op::I32DivS:
        case Op::I32DivU:
        case Op::I32RemS:
        case Op::I32RemU:
        case Op::I32And:
        case Op::I32Or:
        case Op::I32Xor:
        case Op::I32Shl:
        case Op::I32ShrS:
        case Op::I32ShrU:
        case Op::I32Rotl:
        case Op::I32Rotr:
        case Op::I32Eq:
        case Op::I32Ne:
        case Op::I32LtS:
        case Op::I32LtU:
        case Op::I32GtS:
        case Op::I32GtU:
        case Op::I32LeS:
        case Op::I32LeU:
        case Op::I32GeS:
        case Op::I32GeU:
            validator.pop();
            validator.pop();
            validator.push(ValType::i32());
            break;
            
        default:
            // Many more opcodes would be handled in a complete implementation
            break;
    }
    
    return true;
}

// =============================================================================
// Data Section Validation
// =============================================================================

bool ModuleValidator::validateDataSection(Decoder& decoder, uint32_t count) {
    if (dataCount_ && *dataCount_ != count) {
        return setError("data count mismatch", decoder.currentOffset());
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t flags;
        if (!decoder.readVarU32(&flags)) {
            return setError("expected data flags", decoder.currentOffset());
        }
        
        auto kind = static_cast<DataSegmentKind>(flags);
        
        if (kind == DataSegmentKind::ActiveWithMemoryIndex) {
            uint32_t memIdx;
            if (!decoder.readVarU32(&memIdx)) {
                return setError("expected memory index", decoder.currentOffset());
            }
            if (memIdx >= ctx_.numMemories()) {
                return setError("memory index out of bounds", decoder.currentOffset());
            }
        }
        
        if (kind == DataSegmentKind::Active || kind == DataSegmentKind::ActiveWithMemoryIndex) {
            if (!validateConstExpr(decoder, ValType::i32())) {
                return false;
            }
        }
        
        // Read data bytes
        uint32_t dataLen;
        const uint8_t* dataBytes;
        if (!decoder.readBytes(&dataLen, &dataBytes)) {
            return setError("expected data bytes", decoder.currentOffset());
        }
        
        ctx_.addDataSegment();
    }
    return true;
}

// =============================================================================
// Data Count Section Validation
// =============================================================================

bool ModuleValidator::validateDataCountSection(Decoder& decoder) {
    uint32_t count;
    if (!decoder.readVarU32(&count)) {
        return setError("expected data count", decoder.currentOffset());
    }
    dataCount_ = count;
    return true;
}

// =============================================================================
// Tag Section Validation
// =============================================================================

bool ModuleValidator::validateTagSection(Decoder& decoder, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t attribute;
        if (!decoder.readFixedU8(&attribute)) {
            return setError("expected tag attribute", decoder.currentOffset());
        }
        
        uint32_t typeIdx;
        if (!decoder.readVarU32(&typeIdx)) {
            return setError("expected tag type index", decoder.currentOffset());
        }
        
        if (typeIdx >= types_.size()) {
            return setError("tag type index out of bounds", decoder.currentOffset());
        }
        
        ctx_.addTag(typeIdx);
    }
    return true;
}

// =============================================================================
// Constant Expression Validation
// =============================================================================

bool ModuleValidator::validateConstExpr(Decoder& decoder, ValType expectedType) {
    Opcode op;
    while (decoder.readOp(&op)) {
        if (op.isOp() && op.asOp() == Op::End) {
            return true;
        }
        
        // Only limited instructions allowed in const expr
        if (!op.isOp()) {
            return setError("invalid const expr opcode", decoder.currentOffset());
        }
        
        switch (op.asOp()) {
            case Op::I32Const: {
                int32_t value;
                if (!decoder.readVarS32(&value)) return false;
                break;
            }
            case Op::I64Const: {
                int64_t value;
                if (!decoder.readVarS64(&value)) return false;
                break;
            }
            case Op::F32Const: {
                float value;
                if (!decoder.readFixedF32(&value)) return false;
                break;
            }
            case Op::F64Const: {
                double value;
                if (!decoder.readFixedF64(&value)) return false;
                break;
            }
            case Op::GlobalGet: {
                uint32_t idx;
                if (!decoder.readVarU32(&idx)) return false;
                break;
            }
            case Op::RefNull: {
                uint8_t type;
                if (!decoder.readFixedU8(&type)) return false;
                break;
            }
            case Op::RefFunc: {
                uint32_t idx;
                if (!decoder.readVarU32(&idx)) return false;
                break;
            }
            default:
                return setError("invalid const expr opcode", decoder.currentOffset());
        }
    }
    return setError("const expr missing end", decoder.currentOffset());
}

// =============================================================================
// Complete Module Validation
// =============================================================================

bool ModuleValidator::validate(const uint8_t* bytes, size_t length) {
    Decoder decoder(bytes, bytes + length);
    
    // Validate header
    if (!decoder.readModuleHeader()) {
        return setError("invalid module header", 0);
    }
    
    // Process sections in order
    while (!decoder.done()) {
        SectionId id;
        uint32_t size;
        if (!decoder.readSectionHeader(&id, &size)) {
            return setError("invalid section header", decoder.currentOffset());
        }
        
        const uint8_t* sectionEnd = decoder.currentPosition() + size;
        
        // Read section count for vector sections
        uint32_t count = 0;
        if (id != SectionId::Custom && id != SectionId::Start && id != SectionId::DataCount) {
            if (!decoder.readVarU32(&count)) {
                return setError("expected section count", decoder.currentOffset());
            }
        }
        
        switch (id) {
            case SectionId::Custom:
                // Skip custom sections
                decoder.advance(size);
                break;
            case SectionId::Type:
                if (!validateTypeSection(decoder, count)) return false;
                break;
            case SectionId::Import:
                if (!validateImportSection(decoder, count)) return false;
                break;
            case SectionId::Function:
                if (!validateFunctionSection(decoder, count)) return false;
                break;
            case SectionId::Table:
                if (!validateTableSection(decoder, count)) return false;
                break;
            case SectionId::Memory:
                if (!validateMemorySection(decoder, count)) return false;
                break;
            case SectionId::Global:
                if (!validateGlobalSection(decoder, count)) return false;
                break;
            case SectionId::Export:
                if (!validateExportSection(decoder, count)) return false;
                break;
            case SectionId::Start:
                if (!validateStartSection(decoder)) return false;
                break;
            case SectionId::Element:
                if (!validateElementSection(decoder, count)) return false;
                break;
            case SectionId::Code:
                if (!validateCodeSection(decoder, count)) return false;
                break;
            case SectionId::Data:
                if (!validateDataSection(decoder, count)) return false;
                break;
            case SectionId::DataCount:
                if (!validateDataCountSection(decoder)) return false;
                break;
            case SectionId::Tag:
                if (!validateTagSection(decoder, count)) return false;
                break;
            default:
                return setError("unknown section id", decoder.currentOffset());
        }
        
        // Verify section consumed correctly
        if (decoder.currentPosition() != sectionEnd) {
            return setError("section size mismatch", decoder.currentOffset());
        }
    }
    
    // Verify function declarations match bodies
    if (numFunctionBodies_ != numFunctionDecls_) {
        return setError("function declaration/body count mismatch", decoder.currentOffset());
    }
    
    return true;
}

} // namespace Zepra::Wasm
