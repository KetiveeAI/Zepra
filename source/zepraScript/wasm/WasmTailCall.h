/**
 * @file WasmTailCall.h
 * @brief WebAssembly Tail Call Proposal
 * 
 * Implements tail call optimization:
 * - return_call instruction
 * - return_call_indirect instruction
 * - Stack frame reuse
 */

#pragma once

#include <cstdint>

namespace Zepra::Wasm {

// =============================================================================
// Tail Call Opcodes
// =============================================================================

namespace TailCallOp {
    constexpr uint8_t return_call = 0x12;          // Direct tail call
    constexpr uint8_t return_call_indirect = 0x13; // Indirect tail call
}

// =============================================================================
// Tail Call Detection
// =============================================================================

/**
 * @brief Analyzes function for tail call opportunities
 */
class TailCallAnalyzer {
public:
    struct TailCallInfo {
        uint32_t offset;        // Instruction offset
        uint32_t targetFunc;    // Target function index (for direct)
        uint32_t tableIdx;      // Table index (for indirect)
        uint32_t typeIdx;       // Type index (for indirect)
        bool isDirect;          // true = return_call, false = return_call_indirect
    };
    
    // Check if instruction at offset is a tail call
    static bool isTailCall(uint8_t opcode) {
        return opcode == TailCallOp::return_call || 
               opcode == TailCallOp::return_call_indirect;
    }
    
    // Detect tail call pattern: call followed by return
    static bool canOptimizeToTailCall(const uint8_t* code, size_t offset, size_t size) {
        if (offset + 1 >= size) return false;
        
        uint8_t op = code[offset];
        // Check for call (0x10) or call_indirect (0x11) followed by end (0x0B)
        if (op == 0x10 || op == 0x11) {
            // Skip the call instruction operand(s)
            size_t nextOffset = offset + 1;
            
            // Skip LEB128 encoded operand(s)
            while (nextOffset < size && (code[nextOffset] & 0x80)) {
                nextOffset++;
            }
            nextOffset++; // Skip final byte of LEB128
            
            if (op == 0x11) {
                // call_indirect has table index
                while (nextOffset < size && (code[nextOffset] & 0x80)) {
                    nextOffset++;
                }
                nextOffset++;
            }
            
            // Check if next instruction is end
            if (nextOffset < size && code[nextOffset] == 0x0B) {
                return true;
            }
        }
        return false;
    }
};

// =============================================================================
// Tail Call Code Generation
// =============================================================================

/**
 * @brief JIT support for tail calls
 */
class TailCallCodeGen {
public:
    // Generate tail call sequence (x86-64)
    // Instead of: call target; ret
    // Generate:   jmp target (with stack adjustment)
    
    template<typename Assembler>
    static void emitTailCall(Assembler& as, uint32_t targetOffset) {
        // Restore caller's frame
        // mov rsp, rbp
        // pop rbp
        // jmp target
        
        // This is a simplified version - real implementation needs:
        // 1. Adjust stack for argument passing
        // 2. Handle different calling conventions
        // 3. Preserve return address manipulation
    }
    
    template<typename Assembler>
    static void emitTailCallIndirect(Assembler& as, uint8_t tableReg, uint32_t typeIdx) {
        // Similar to emitTailCall but with indirect target
        // mov rsp, rbp
        // pop rbp
        // jmp [tableReg]
    }
    
    // Stack adjustment calculation
    static int32_t calculateStackDelta(uint32_t callerParams, uint32_t calleeParams) {
        // Caller needs to adjust stack if callee has different param count
        return static_cast<int32_t>(calleeParams - callerParams) * 8;
    }
};

// =============================================================================
// Tail Call Validation
// =============================================================================

/**
 * @brief Validates tail call constraints
 */
class TailCallValidator {
public:
    // Type must match exactly for tail call
    static bool validateTailCall(uint32_t callerTypeIdx, uint32_t calleeTypeIdx,
                                 const void* types) {
        // Types must be identical for tail call to be valid
        return callerTypeIdx == calleeTypeIdx;
    }
    
    // Check if function can use tail calls
    static bool canUseTailCalls(uint32_t funcIdx, const void* module) {
        // All functions can potentially use tail calls
        // Real implementation would check for:
        // 1. No catch blocks that need cleanup
        // 2. No finally blocks
        // 3. Compatible return types
        return true;
    }
};

// =============================================================================
// Extended Const Expressions (34b)
// =============================================================================

/**
 * @brief Extended constant expression evaluator
 */
class ExtendedConstExpr {
public:
    enum class ConstOp : uint8_t {
        i32_const = 0x41,
        i64_const = 0x42,
        f32_const = 0x43,
        f64_const = 0x44,
        
        // Extended const ops
        i32_add = 0x6A,
        i32_sub = 0x6B,
        i32_mul = 0x6C,
        i64_add = 0x7C,
        i64_sub = 0x7D,
        i64_mul = 0x7E,
        
        global_get = 0x23,
        ref_null = 0xD0,
        ref_func = 0xD2
    };
    
    // Evaluate constant expression
    static int64_t evaluate(const uint8_t* expr, size_t size) {
        std::vector<int64_t> stack;
        size_t i = 0;
        
        while (i < size) {
            uint8_t op = expr[i++];
            
            switch (static_cast<ConstOp>(op)) {
                case ConstOp::i32_const: {
                    int32_t val = readSignedLEB128(expr, i);
                    stack.push_back(val);
                    break;
                }
                case ConstOp::i64_const: {
                    int64_t val = readSignedLEB128_64(expr, i);
                    stack.push_back(val);
                    break;
                }
                case ConstOp::i32_add:
                case ConstOp::i64_add: {
                    int64_t b = stack.back(); stack.pop_back();
                    int64_t a = stack.back(); stack.pop_back();
                    stack.push_back(a + b);
                    break;
                }
                case ConstOp::i32_sub:
                case ConstOp::i64_sub: {
                    int64_t b = stack.back(); stack.pop_back();
                    int64_t a = stack.back(); stack.pop_back();
                    stack.push_back(a - b);
                    break;
                }
                case ConstOp::i32_mul:
                case ConstOp::i64_mul: {
                    int64_t b = stack.back(); stack.pop_back();
                    int64_t a = stack.back(); stack.pop_back();
                    stack.push_back(a * b);
                    break;
                }
                default:
                    if (op == 0x0B) return stack.back(); // end
                    break;
            }
        }
        
        return stack.empty() ? 0 : stack.back();
    }
    
private:
    static int32_t readSignedLEB128(const uint8_t* data, size_t& offset) {
        int32_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            byte = data[offset++];
            result |= (byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        if (shift < 32 && (byte & 0x40)) {
            result |= (~0 << shift);
        }
        return result;
    }
    
    static int64_t readSignedLEB128_64(const uint8_t* data, size_t& offset) {
        int64_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            byte = data[offset++];
            result |= static_cast<int64_t>(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        if (shift < 64 && (byte & 0x40)) {
            result |= (~0LL << shift);
        }
        return result;
    }
};

// =============================================================================
// Custom Annotations (34c)
// =============================================================================

/**
 * @brief Custom section handler for annotations
 */
class CustomAnnotations {
public:
    struct Annotation {
        std::string name;
        std::vector<uint8_t> data;
    };
    
    void addAnnotation(const std::string& name, const uint8_t* data, size_t size) {
        annotations_.push_back({name, std::vector<uint8_t>(data, data + size)});
    }
    
    const Annotation* getAnnotation(const std::string& name) const {
        for (const auto& ann : annotations_) {
            if (ann.name == name) return &ann;
        }
        return nullptr;
    }
    
    // Known annotation names
    static constexpr const char* NAME_SECTION = "name";
    static constexpr const char* SOURCE_MAP = "sourceMappingURL";
    static constexpr const char* PRODUCERS = "producers";
    static constexpr const char* TARGET_FEATURES = "target_features";
    
    // Parse name section for debugging
    void parseNameSection(const uint8_t* data, size_t size) {
        // Parse module name, function names, local names
        // Format: subsection_id(u8) + size(u32) + payload
    }
    
    const std::vector<Annotation>& all() const { return annotations_; }
    
private:
    std::vector<Annotation> annotations_;
};

} // namespace Zepra::Wasm
