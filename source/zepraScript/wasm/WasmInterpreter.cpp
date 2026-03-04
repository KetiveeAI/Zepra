/**
 * @file WasmInterpreter.cpp
 * @brief WebAssembly bytecode interpreter implementation
 * 
 * Implements the WasmInterpreter class defined in wasm.hpp
 */

#include "wasm/wasm.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace Zepra::Wasm {

// Forward declarations of helper functions
static uint32_t readVarU32(const uint8_t*& ip) {
    uint32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ip++;
        result |= (byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    return result;
}

static int32_t readVarI32(const uint8_t*& ip) {
    int32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ip++;
        result |= (byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    if ((shift < 32) && (byte & 0x40)) {
        result |= (~0u << shift);
    }
    return result;
}

static int64_t readVarI64(const uint8_t*& ip) {
    int64_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = *ip++;
        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    if ((shift < 64) && (byte & 0x40)) {
        result |= (~0LL << shift);
    }
    return result;
}

WasmInterpreter::WasmInterpreter(WasmInstance* instance)
    : instance_(instance) {
    stack_.reserve(4096);
    controlStack_.reserve(256);
}

WasmValue WasmInterpreter::execute(uint32_t funcIdx, const std::vector<WasmValue>& args) {
    WasmModule* module = instance_->module_;
    
    if (funcIdx >= module->code().size()) {
        throw std::runtime_error("Invalid function index");
    }
    
    const auto& funcCode = module->code()[funcIdx];
    
    // Initialize locals from args
    locals_.clear();
    for (const auto& arg : args) {
        locals_.push_back(arg);
    }
    
    // Add declared locals (zero-initialized)
    for (const auto& localDecl : funcCode.locals) {
        for (uint32_t i = 0; i < localDecl.first; i++) {
            WasmValue v;
            v.type = localDecl.second;
            v.i64 = 0;
            locals_.push_back(v);
        }
    }
    
    // Execute bytecode
    const uint8_t* ip = funcCode.body.data();
    const uint8_t* end = ip + funcCode.body.size();
    
    while (ip < end) {
        uint8_t opcode = *ip++;
        executeInstruction(opcode, ip);
        
        if (opcode == Opcode::END && controlStack_.empty()) {
            break;
        }
    }
    
    // Return result (or undefined)
    if (!stack_.empty()) {
        return pop();
    }
    
    WasmValue empty;
    empty.type = ValType::I32;
    empty.i32 = 0;
    return empty;
}

void WasmInterpreter::executeInstruction(uint8_t opcode, const uint8_t*& ip) {
    switch (opcode) {
        case Opcode::UNREACHABLE:
            throw std::runtime_error("Unreachable executed");
            
        case Opcode::NOP:
            break;
        
        // Exception handling opcodes
        case Opcode::TRY: {
            // Skip block type
            uint8_t bt = *ip++;
            (void)bt;
            // Push try block onto control stack with exception handler info
            Label label;
            label.arity = 0;
            label.stackHeight = stack_.size();
            label.continuation = nullptr;
            label.isTry = true;
            controlStack_.push_back(label);
            break;
        }
        
        case Opcode::CATCH: {
            // Exception tag index
            uint32_t tagIdx = readVarU32(ip);
            (void)tagIdx;
            // If we reach catch normally (no throw), skip to end
            // Pop try label, push catch label
            if (!controlStack_.empty()) {
                controlStack_.pop_back();
            }
            Label label;
            label.arity = 0;
            label.stackHeight = stack_.size();
            label.continuation = nullptr;
            label.isCatch = true;
            controlStack_.push_back(label);
            break;
        }
        
        case Opcode::CATCH_ALL: {
            // Catch all exceptions
            if (!controlStack_.empty()) {
                controlStack_.pop_back();
            }
            Label label;
            label.arity = 0;
            label.stackHeight = stack_.size();
            label.continuation = nullptr;
            label.isCatch = true;
            controlStack_.push_back(label);
            break;
        }
        
        case Opcode::THROW: {
            uint32_t tagIdx = readVarU32(ip);
            // Throw exception with tag
            throw std::runtime_error("WASM exception thrown with tag " + std::to_string(tagIdx));
        }
        
        case Opcode::RETHROW: {
            uint32_t depth = readVarU32(ip);
            (void)depth;
            // Rethrow current exception
            throw std::runtime_error("WASM exception rethrown");
        }
        
        case Opcode::DELEGATE: {
            uint32_t depth = readVarU32(ip);
            (void)depth;
            // Delegate exception to outer try block
            break;
        }
            
        case Opcode::BLOCK:
        case Opcode::LOOP:
        case Opcode::IF: {
            // Skip block type
            uint8_t bt = *ip++;
            (void)bt;
            Label label;
            label.arity = 0;
            label.stackHeight = stack_.size();
            label.continuation = nullptr;
            controlStack_.push_back(label);
            break;
        }
            
        case Opcode::ELSE:
            break;
            
        case Opcode::END:
            if (!controlStack_.empty()) {
                controlStack_.pop_back();
            }
            break;
            
        case Opcode::BR: {
            uint32_t depth = readVarU32(ip);
            for (uint32_t i = 0; i <= depth && !controlStack_.empty(); i++) {
                controlStack_.pop_back();
            }
            break;
        }
            
        case Opcode::BR_IF: {
            uint32_t depth = readVarU32(ip);
            WasmValue cond = pop();
            if (cond.i32 != 0) {
                for (uint32_t i = 0; i <= depth && !controlStack_.empty(); i++) {
                    controlStack_.pop_back();
                }
            }
            break;
        }
            
        case Opcode::RETURN:
            controlStack_.clear();
            break;
            
        case Opcode::CALL: {
            uint32_t funcIdx = readVarU32(ip);
            WasmModule* module = instance_->module_;
            
            // Get the function type
            if (funcIdx >= module->funcTypeIndices_.size()) {
                throw std::runtime_error("Invalid function index");
            }
            uint32_t typeIdx = module->funcTypeIndices_[funcIdx];
            const FuncType& funcType = module->types_[typeIdx];
            
            // Pop arguments from stack (in reverse order)
            std::vector<WasmValue> args(funcType.params.size());
            for (int i = static_cast<int>(funcType.params.size()) - 1; i >= 0; i--) {
                args[i] = pop();
            }
            
            // Save current state
            std::vector<WasmValue> savedLocals = std::move(locals_);
            std::vector<Label> savedControlStack = std::move(controlStack_);
            
            // Execute the function
            WasmValue result = execute(funcIdx, args);
            
            // Restore state
            locals_ = std::move(savedLocals);
            controlStack_ = std::move(savedControlStack);
            
            // Push result if function returns a value
            if (!funcType.results.empty()) {
                push(result);
            }
            break;
        }
            
        case Opcode::DROP:
            pop();
            break;
            
        case Opcode::SELECT: {
            WasmValue c = pop();
            WasmValue b = pop();
            WasmValue a = pop();
            push(c.i32 ? a : b);
            break;
        }
            
        case Opcode::LOCAL_GET: {
            uint32_t idx = readVarU32(ip);
            push(locals_[idx]);
            break;
        }
            
        case Opcode::LOCAL_SET: {
            uint32_t idx = readVarU32(ip);
            locals_[idx] = pop();
            break;
        }
            
        case Opcode::LOCAL_TEE: {
            uint32_t idx = readVarU32(ip);
            locals_[idx] = stack_.back();
            break;
        }
            
        case Opcode::GLOBAL_GET: {
            uint32_t idx = readVarU32(ip);
            push(instance_->getGlobal(idx)->getValue());
            break;
        }
            
        case Opcode::GLOBAL_SET: {
            uint32_t idx = readVarU32(ip);
            instance_->getGlobal(idx)->setValue(pop());
            break;
        }
        
        // Memory load operations
        case Opcode::I32_LOAD: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            push(WasmValue::fromI32(mem->load<int32_t>(addr)));
            break;
        }
        case Opcode::I64_LOAD: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            push(WasmValue::fromI64(mem->load<int64_t>(addr)));
            break;
        }
        case Opcode::F32_LOAD: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            push(WasmValue::fromF32(mem->load<float>(addr)));
            break;
        }
        case Opcode::F64_LOAD: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            push(WasmValue::fromF64(mem->load<double>(addr)));
            break;
        }
        
        // Memory store operations
        case Opcode::I32_STORE: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto value = pop();
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            mem->store<int32_t>(addr, value.i32);
            break;
        }
        case Opcode::I64_STORE: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto value = pop();
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            mem->store<int64_t>(addr, value.i64);
            break;
        }
        case Opcode::F32_STORE: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto value = pop();
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            mem->store<float>(addr, value.f32);
            break;
        }
        case Opcode::F64_STORE: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto value = pop();
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            mem->store<double>(addr, value.f64);
            break;
        }
        
        // Memory size/grow
        case Opcode::MEMORY_SIZE: {
            readVarU32(ip); // memory index (always 0 in MVP)
            WasmMemory* mem = instance_->getMemory(0);
            push(WasmValue::fromI32(static_cast<int32_t>(mem->pageCount())));
            break;
        }
        case Opcode::MEMORY_GROW: {
            readVarU32(ip); // memory index
            auto delta = pop();
            WasmMemory* mem = instance_->getMemory(0);
            size_t oldPages = mem->grow(delta.i32);
            push(WasmValue::fromI32(oldPages == static_cast<size_t>(-1) ? -1 : static_cast<int32_t>(oldPages)));
            break;
        }
            
        case Opcode::I32_CONST: {
            int32_t val = readVarI32(ip);
            push(WasmValue::fromI32(val));
            break;
        }
            
        case Opcode::I64_CONST: {
            int64_t val = readVarI64(ip);
            push(WasmValue::fromI64(val));
            break;
        }
            
        case Opcode::F32_CONST: {
            float val;
            memcpy(&val, ip, 4);
            ip += 4;
            push(WasmValue::fromF32(val));
            break;
        }
            
        case Opcode::F64_CONST: {
            double val;
            memcpy(&val, ip, 8);
            ip += 8;
            push(WasmValue::fromF64(val));
            break;
        }
            
        // I32 comparisons
        case Opcode::I32_EQZ: {
            auto a = pop();
            push(WasmValue::fromI32(a.i32 == 0 ? 1 : 0));
            break;
        }
        case Opcode::I32_EQ: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 == b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_NE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 != b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_LT_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 < b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_LT_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint32_t>(a.i32) < static_cast<uint32_t>(b.i32) ? 1 : 0));
            break;
        }
        case Opcode::I32_GT_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 > b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_GT_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint32_t>(a.i32) > static_cast<uint32_t>(b.i32) ? 1 : 0));
            break;
        }
        case Opcode::I32_LE_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 <= b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_LE_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint32_t>(a.i32) <= static_cast<uint32_t>(b.i32) ? 1 : 0));
            break;
        }
        case Opcode::I32_GE_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 >= b.i32 ? 1 : 0));
            break;
        }
        case Opcode::I32_GE_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint32_t>(a.i32) >= static_cast<uint32_t>(b.i32) ? 1 : 0));
            break;
        }
            
        // I32 arithmetic
        case Opcode::I32_ADD: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 + b.i32));
            break;
        }
        case Opcode::I32_SUB: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 - b.i32));
            break;
        }
        case Opcode::I32_MUL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 * b.i32));
            break;
        }
        case Opcode::I32_DIV_S: {
            auto b = pop(); auto a = pop();
            if (b.i32 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI32(a.i32 / b.i32));
            break;
        }
        case Opcode::I32_DIV_U: {
            auto b = pop(); auto a = pop();
            if (b.i32 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI32(static_cast<int32_t>(
                static_cast<uint32_t>(a.i32) / static_cast<uint32_t>(b.i32))));
            break;
        }
        case Opcode::I32_REM_S: {
            auto b = pop(); auto a = pop();
            if (b.i32 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI32(a.i32 % b.i32));
            break;
        }
        case Opcode::I32_REM_U: {
            auto b = pop(); auto a = pop();
            if (b.i32 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI32(static_cast<int32_t>(
                static_cast<uint32_t>(a.i32) % static_cast<uint32_t>(b.i32))));
            break;
        }
        case Opcode::I32_AND: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 & b.i32));
            break;
        }
        case Opcode::I32_OR: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 | b.i32));
            break;
        }
        case Opcode::I32_XOR: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 ^ b.i32));
            break;
        }
        case Opcode::I32_SHL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 << (b.i32 & 31)));
            break;
        }
        case Opcode::I32_SHR_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i32 >> (b.i32 & 31)));
            break;
        }
        case Opcode::I32_SHR_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(
                static_cast<uint32_t>(a.i32) >> (b.i32 & 31))));
            break;
        }
        
        // i64 comparisons
        case Opcode::I64_EQZ: {
            auto a = pop();
            push(WasmValue::fromI32(a.i64 == 0 ? 1 : 0));
            break;
        }
        case Opcode::I64_EQ: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i64 == b.i64 ? 1 : 0));
            break;
        }
        case Opcode::I64_NE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i64 != b.i64 ? 1 : 0));
            break;
        }
        case Opcode::I64_LT_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i64 < b.i64 ? 1 : 0));
            break;
        }
        case Opcode::I64_LT_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint64_t>(a.i64) < static_cast<uint64_t>(b.i64) ? 1 : 0));
            break;
        }
        case Opcode::I64_GT_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.i64 > b.i64 ? 1 : 0));
            break;
        }
        case Opcode::I64_GT_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(static_cast<uint64_t>(a.i64) > static_cast<uint64_t>(b.i64) ? 1 : 0));
            break;
        }
        
        // i64 arithmetic
        case Opcode::I64_ADD: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 + b.i64));
            break;
        }
        case Opcode::I64_SUB: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 - b.i64));
            break;
        }
        case Opcode::I64_MUL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 * b.i64));
            break;
        }
        case Opcode::I64_DIV_S: {
            auto b = pop(); auto a = pop();
            if (b.i64 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI64(a.i64 / b.i64));
            break;
        }
        case Opcode::I64_AND: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 & b.i64));
            break;
        }
        case Opcode::I64_OR: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 | b.i64));
            break;
        }
        case Opcode::I64_XOR: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 ^ b.i64));
            break;
        }
        case Opcode::I64_SHL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 << (b.i64 & 63)));
            break;
        }
        case Opcode::I64_SHR_S: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(a.i64 >> (b.i64 & 63)));
            break;
        }
        case Opcode::I64_SHR_U: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(
                static_cast<uint64_t>(a.i64) >> (b.i64 & 63))));
            break;
        }
        
        // f32 comparisons
        case Opcode::F32_EQ: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 == b.f32 ? 1 : 0));
            break;
        }
        case Opcode::F32_NE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 != b.f32 ? 1 : 0));
            break;
        }
        case Opcode::F32_LT: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 < b.f32 ? 1 : 0));
            break;
        }
        case Opcode::F32_GT: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 > b.f32 ? 1 : 0));
            break;
        }
        case Opcode::F32_LE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 <= b.f32 ? 1 : 0));
            break;
        }
        case Opcode::F32_GE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f32 >= b.f32 ? 1 : 0));
            break;
        }
        
        // f32 arithmetic
        case Opcode::F32_ABS: {
            auto a = pop();
            push(WasmValue::fromF32(std::fabs(a.f32)));
            break;
        }
        case Opcode::F32_NEG: {
            auto a = pop();
            push(WasmValue::fromF32(-a.f32));
            break;
        }
        case Opcode::F32_SQRT: {
            auto a = pop();
            push(WasmValue::fromF32(std::sqrt(a.f32)));
            break;
        }
        case Opcode::F32_ADD: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(a.f32 + b.f32));
            break;
        }
        case Opcode::F32_SUB: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(a.f32 - b.f32));
            break;
        }
        case Opcode::F32_MUL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(a.f32 * b.f32));
            break;
        }
        case Opcode::F32_DIV: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(a.f32 / b.f32));
            break;
        }
        case Opcode::F32_MIN: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(std::fmin(a.f32, b.f32)));
            break;
        }
        case Opcode::F32_MAX: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF32(std::fmax(a.f32, b.f32)));
            break;
        }
        
        // f64 comparisons
        case Opcode::F64_EQ: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 == b.f64 ? 1 : 0));
            break;
        }
        case Opcode::F64_NE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 != b.f64 ? 1 : 0));
            break;
        }
        case Opcode::F64_LT: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 < b.f64 ? 1 : 0));
            break;
        }
        case Opcode::F64_GT: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 > b.f64 ? 1 : 0));
            break;
        }
        case Opcode::F64_LE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 <= b.f64 ? 1 : 0));
            break;
        }
        case Opcode::F64_GE: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromI32(a.f64 >= b.f64 ? 1 : 0));
            break;
        }
        
        // f64 arithmetic
        case Opcode::F64_ABS: {
            auto a = pop();
            push(WasmValue::fromF64(std::fabs(a.f64)));
            break;
        }
        case Opcode::F64_NEG: {
            auto a = pop();
            push(WasmValue::fromF64(-a.f64));
            break;
        }
        case Opcode::F64_SQRT: {
            auto a = pop();
            push(WasmValue::fromF64(std::sqrt(a.f64)));
            break;
        }
        case Opcode::F64_ADD: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(a.f64 + b.f64));
            break;
        }
        case Opcode::F64_SUB: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(a.f64 - b.f64));
            break;
        }
        case Opcode::F64_MUL: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(a.f64 * b.f64));
            break;
        }
        case Opcode::F64_DIV: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(a.f64 / b.f64));
            break;
        }
        case Opcode::F64_MIN: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(std::fmin(a.f64, b.f64)));
            break;
        }
        case Opcode::F64_MAX: {
            auto b = pop(); auto a = pop();
            push(WasmValue::fromF64(std::fmax(a.f64, b.f64)));
            break;
        }
        
        // Conversions
        case Opcode::I32_WRAP_I64: {
            auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(a.i64)));
            break;
        }
        case Opcode::I32_TRUNC_F32_S: {
            auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(a.f32)));
            break;
        }
        case Opcode::I32_TRUNC_F32_U: {
            auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(static_cast<uint32_t>(a.f32))));
            break;
        }
        case Opcode::I32_TRUNC_F64_S: {
            auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(a.f64)));
            break;
        }
        case Opcode::I32_TRUNC_F64_U: {
            auto a = pop();
            push(WasmValue::fromI32(static_cast<int32_t>(static_cast<uint32_t>(a.f64))));
            break;
        }
        case Opcode::I64_EXTEND_I32_S: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(a.i32)));
            break;
        }
        case Opcode::I64_EXTEND_I32_U: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(static_cast<uint32_t>(a.i32))));
            break;
        }
        case Opcode::I64_TRUNC_F32_S: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(a.f32)));
            break;
        }
        case Opcode::I64_TRUNC_F32_U: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(static_cast<uint64_t>(a.f32))));
            break;
        }
        case Opcode::I64_TRUNC_F64_S: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(a.f64)));
            break;
        }
        case Opcode::I64_TRUNC_F64_U: {
            auto a = pop();
            push(WasmValue::fromI64(static_cast<int64_t>(static_cast<uint64_t>(a.f64))));
            break;
        }
        case Opcode::F32_CONVERT_I32_S: {
            auto a = pop();
            push(WasmValue::fromF32(static_cast<float>(a.i32)));
            break;
        }
        case Opcode::F32_CONVERT_I32_U: {
            auto a = pop();
            push(WasmValue::fromF32(static_cast<float>(static_cast<uint32_t>(a.i32))));
            break;
        }
        case Opcode::F32_CONVERT_I64_S: {
            auto a = pop();
            push(WasmValue::fromF32(static_cast<float>(a.i64)));
            break;
        }
        case Opcode::F32_CONVERT_I64_U: {
            auto a = pop();
            push(WasmValue::fromF32(static_cast<float>(static_cast<uint64_t>(a.i64))));
            break;
        }
        case Opcode::F32_DEMOTE_F64: {
            auto a = pop();
            push(WasmValue::fromF32(static_cast<float>(a.f64)));
            break;
        }
        case Opcode::F64_CONVERT_I32_S: {
            auto a = pop();
            push(WasmValue::fromF64(static_cast<double>(a.i32)));
            break;
        }
        case Opcode::F64_CONVERT_I32_U: {
            auto a = pop();
            push(WasmValue::fromF64(static_cast<double>(static_cast<uint32_t>(a.i32))));
            break;
        }
        case Opcode::F64_CONVERT_I64_S: {
            auto a = pop();
            push(WasmValue::fromF64(static_cast<double>(a.i64)));
            break;
        }
        case Opcode::F64_CONVERT_I64_U: {
            auto a = pop();
            push(WasmValue::fromF64(static_cast<double>(static_cast<uint64_t>(a.i64))));
            break;
        }
        case Opcode::F64_PROMOTE_F32: {
            auto a = pop();
            push(WasmValue::fromF64(static_cast<double>(a.f32)));
            break;
        }
        
        // Reinterpretations
        case Opcode::I32_REINTERPRET_F32: {
            auto a = pop();
            int32_t result;
            std::memcpy(&result, &a.f32, 4);
            push(WasmValue::fromI32(result));
            break;
        }
        case Opcode::I64_REINTERPRET_F64: {
            auto a = pop();
            int64_t result;
            std::memcpy(&result, &a.f64, 8);
            push(WasmValue::fromI64(result));
            break;
        }
        case Opcode::F32_REINTERPRET_I32: {
            auto a = pop();
            float result;
            std::memcpy(&result, &a.i32, 4);
            push(WasmValue::fromF32(result));
            break;
        }
        case Opcode::F64_REINTERPRET_I64: {
            auto a = pop();
            double result;
            std::memcpy(&result, &a.i64, 8);
            push(WasmValue::fromF64(result));
            break;
        }
        
        // SIMD prefix (0xFD)
        case Opcode::SIMD_PREFIX: {
            uint32_t simdOp = readVarU32(ip);
            executeSimdInstruction(simdOp, ip);
            break;
        }
            
        default:
            // Unimplemented opcode - skip
            break;
    }
}

void WasmInterpreter::executeSimdInstruction(uint32_t simdOp, const uint8_t*& ip) {
    using namespace Opcode::Simd;
    
    switch (simdOp) {
        case V128_LOAD: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = mem->load<uint8_t>(addr + i);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case V128_STORE: {
            uint32_t align = readVarU32(ip);
            uint32_t offset = readVarU32(ip);
            (void)align;
            auto value = pop();
            auto base = pop();
            uint32_t addr = static_cast<uint32_t>(base.i32) + offset;
            WasmMemory* mem = instance_->getMemory(0);
            for (int i = 0; i < 16; i++) {
                mem->store<uint8_t>(addr + i, value.v128[i]);
            }
            break;
        }
        
        case V128_CONST: {
            uint8_t data[16];
            std::memcpy(data, ip, 16);
            ip += 16;
            push(WasmValue::fromV128(data));
            break;
        }
        
        // Splat operations
        case I8X16_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = static_cast<uint8_t>(a.i32);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I16X8_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            int16_t val = static_cast<int16_t>(a.i32);
            for (int i = 0; i < 8; i++) {
                std::memcpy(data + i * 2, &val, 2);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                std::memcpy(data + i * 4, &a.i32, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I64X2_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            std::memcpy(data, &a.i64, 8);
            std::memcpy(data + 8, &a.i64, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                std::memcpy(data + i * 4, &a.f32, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F64X2_SPLAT: {
            auto a = pop();
            uint8_t data[16];
            std::memcpy(data, &a.f64, 8);
            std::memcpy(data + 8, &a.f64, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        // Extract lane
        case I32X4_EXTRACT_LANE: {
            uint8_t lane = *ip++;
            auto a = pop();
            int32_t val;
            std::memcpy(&val, a.v128 + lane * 4, 4);
            push(WasmValue::fromI32(val));
            break;
        }
        
        case I64X2_EXTRACT_LANE: {
            uint8_t lane = *ip++;
            auto a = pop();
            int64_t val;
            std::memcpy(&val, a.v128 + lane * 8, 8);
            push(WasmValue::fromI64(val));
            break;
        }
        
        case F32X4_EXTRACT_LANE: {
            uint8_t lane = *ip++;
            auto a = pop();
            float val;
            std::memcpy(&val, a.v128 + lane * 4, 4);
            push(WasmValue::fromF32(val));
            break;
        }
        
        case F64X2_EXTRACT_LANE: {
            uint8_t lane = *ip++;
            auto a = pop();
            double val;
            std::memcpy(&val, a.v128 + lane * 8, 8);
            push(WasmValue::fromF64(val));
            break;
        }
        
        // Replace lane
        case I32X4_REPLACE_LANE: {
            uint8_t lane = *ip++;
            auto val = pop();
            auto vec = pop();
            std::memcpy(vec.v128 + lane * 4, &val.i32, 4);
            push(vec);
            break;
        }
        
        // Bitwise operations
        case V128_NOT: {
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = ~a.v128[i];
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case V128_AND: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = a.v128[i] & b.v128[i];
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case V128_OR: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = a.v128[i] | b.v128[i];
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case V128_XOR: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                data[i] = a.v128[i] ^ b.v128[i];
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        // i32x4 arithmetic
        case I32X4_ADD: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = va + vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_SUB: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = va - vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_MUL: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = va * vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        // f32x4 arithmetic
        case F32X4_ADD: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                float res = va + vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_SUB: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                float res = va - vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_MUL: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                float res = va * vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_DIV: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                float res = va / vb;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        // f64x2 arithmetic
        case F64X2_ADD: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            double va0, va1, vb0, vb1;
            std::memcpy(&va0, a.v128, 8);
            std::memcpy(&va1, a.v128 + 8, 8);
            std::memcpy(&vb0, b.v128, 8);
            std::memcpy(&vb1, b.v128 + 8, 8);
            double r0 = va0 + vb0, r1 = va1 + vb1;
            std::memcpy(data, &r0, 8);
            std::memcpy(data + 8, &r1, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F64X2_SUB: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            double va0, va1, vb0, vb1;
            std::memcpy(&va0, a.v128, 8);
            std::memcpy(&va1, a.v128 + 8, 8);
            std::memcpy(&vb0, b.v128, 8);
            std::memcpy(&vb1, b.v128 + 8, 8);
            double r0 = va0 - vb0, r1 = va1 - vb1;
            std::memcpy(data, &r0, 8);
            std::memcpy(data + 8, &r1, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F64X2_MUL: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            double va0, va1, vb0, vb1;
            std::memcpy(&va0, a.v128, 8);
            std::memcpy(&va1, a.v128 + 8, 8);
            std::memcpy(&vb0, b.v128, 8);
            std::memcpy(&vb1, b.v128 + 8, 8);
            double r0 = va0 * vb0, r1 = va1 * vb1;
            std::memcpy(data, &r0, 8);
            std::memcpy(data + 8, &r1, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F64X2_DIV: {
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            double va0, va1, vb0, vb1;
            std::memcpy(&va0, a.v128, 8);
            std::memcpy(&va1, a.v128 + 8, 8);
            std::memcpy(&vb0, b.v128, 8);
            std::memcpy(&vb1, b.v128 + 8, 8);
            double r0 = va0 / vb0, r1 = va1 / vb1;
            std::memcpy(data, &r0, 8);
            std::memcpy(data + 8, &r1, 8);
            push(WasmValue::fromV128(data));
            break;
        }
        
        // SIMD comparisons
        case I32X4_EQ: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va == vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_NE: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va != vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_LT_S: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va < vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_GT_S: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va > vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_LE_S: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va <= vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case I32X4_GE_S: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                int32_t va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va >= vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_EQ: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va == vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_NE: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va != vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_LT: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va < vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_GT: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va > vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_LE: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va <= vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        case F32X4_GE: {
            auto b = pop(); auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 4; i++) {
                float va, vb;
                std::memcpy(&va, a.v128 + i * 4, 4);
                std::memcpy(&vb, b.v128 + i * 4, 4);
                int32_t res = (va >= vb) ? -1 : 0;
                std::memcpy(data + i * 4, &res, 4);
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        // i8x16.shuffle - 16-byte lane selection from two vectors
        case I8X16_SHUFFLE: {
            uint8_t lanes[16];
            std::memcpy(lanes, ip, 16);
            ip += 16;
            auto b = pop();
            auto a = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                uint8_t idx = lanes[i];
                if (idx < 16) {
                    data[i] = a.v128[idx];
                } else {
                    data[i] = b.v128[idx - 16];
                }
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        // i8x16.swizzle - byte selection from one vector
        case I8X16_SWIZZLE: {
            auto indices = pop();
            auto vec = pop();
            uint8_t data[16];
            for (int i = 0; i < 16; i++) {
                uint8_t idx = indices.v128[i];
                data[i] = (idx < 16) ? vec.v128[idx] : 0;
            }
            push(WasmValue::fromV128(data));
            break;
        }
        
        default:
            // Unimplemented SIMD opcode
            break;
    }
}

} // namespace Zepra::Wasm

