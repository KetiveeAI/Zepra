/**
 * @file DFGValue.h
 * @brief DFG SSA Value representation
 * 
 * Core value type for the Data Flow Graph IR.
 * Each value represents a computation result in SSA form.
 */

#pragma once

#include "DFGOpcode.h"
#include <vector>
#include <cstdint>
#include <cassert>

namespace Zepra::DFG {

// Forward declarations
class BasicBlock;
class Graph;

// =============================================================================
// Value Type (mirrors WASM ValType)
// =============================================================================

enum class Type : uint8_t {
    Void,
    I32,
    I64,
    F32,
    F64,
    V128,
    FuncRef,
    ExternRef
};

inline const char* typeName(Type t) {
    switch (t) {
        case Type::Void: return "void";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::F32: return "f32";
        case Type::F64: return "f64";
        case Type::V128: return "v128";
        case Type::FuncRef: return "funcref";
        case Type::ExternRef: return "externref";
    }
    return "unknown";
}

inline bool isInteger(Type t) {
    return t == Type::I32 || t == Type::I64;
}

inline bool isFloat(Type t) {
    return t == Type::F32 || t == Type::F64;
}

inline uint32_t sizeInBytes(Type t) {
    switch (t) {
        case Type::I32: case Type::F32: return 4;
        case Type::I64: case Type::F64: return 8;
        case Type::V128: return 16;
        default: return 0;
    }
}

// =============================================================================
// Constant Data Union
// =============================================================================

union ConstantValue {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    
    ConstantValue() : i64(0) {}
    explicit ConstantValue(int32_t v) : i32(v) {}
    explicit ConstantValue(int64_t v) : i64(v) {}
    explicit ConstantValue(float v) : f32(v) {}
    explicit ConstantValue(double v) : f64(v) {}
};

// =============================================================================
// DFG Value - SSA Value Node
// =============================================================================

class Value {
public:
    // =========================================================================
    // Construction
    // =========================================================================
    
    Value(Opcode op, Type type, uint32_t index)
        : op_(op)
        , type_(type)
        , index_(index)
        , block_(nullptr)
        , flags_(0) {}
    
    // =========================================================================
    // Accessors
    // =========================================================================
    
    Opcode opcode() const { return op_; }
    Type type() const { return type_; }
    uint32_t index() const { return index_; }
    BasicBlock* block() const { return block_; }
    
    void setBlock(BasicBlock* b) { block_ = b; }
    void setIndex(uint32_t idx) { index_ = idx; }
    
    // =========================================================================
    // Inputs (operands)
    // =========================================================================
    
    uint32_t numInputs() const { return static_cast<uint32_t>(inputs_.size()); }
    
    Value* input(uint32_t idx) const {
        assert(idx < inputs_.size());
        return inputs_[idx];
    }
    
    void addInput(Value* v) {
        inputs_.push_back(v);
        if (v) v->addUser(this);
    }
    
    void setInput(uint32_t idx, Value* v) {
        assert(idx < inputs_.size());
        if (inputs_[idx]) inputs_[idx]->removeUser(this);
        inputs_[idx] = v;
        if (v) v->addUser(this);
    }
    
    void clearInputs() {
        for (Value* v : inputs_) {
            if (v) v->removeUser(this);
        }
        inputs_.clear();
    }
    
    const std::vector<Value*>& inputs() const { return inputs_; }
    
    // =========================================================================
    // Users (def-use chain)
    // =========================================================================
    
    uint32_t numUsers() const { return static_cast<uint32_t>(users_.size()); }
    const std::vector<Value*>& users() const { return users_; }
    
    void addUser(Value* u) { users_.push_back(u); }
    
    void removeUser(Value* u) {
        for (auto it = users_.begin(); it != users_.end(); ++it) {
            if (*it == u) {
                users_.erase(it);
                return;
            }
        }
    }
    
    bool hasUsers() const { return !users_.empty(); }
    bool hasNoUsers() const { return users_.empty(); }
    
    // =========================================================================
    // Constant Values
    // =========================================================================
    
    bool isConstant() const {
        return op_ == Opcode::Const32 || op_ == Opcode::Const64 ||
               op_ == Opcode::ConstF32 || op_ == Opcode::ConstF64;
    }
    
    void setConstant(ConstantValue cv) { constant_ = cv; }
    ConstantValue constant() const { return constant_; }
    
    int32_t asInt32() const { assert(isConstant()); return constant_.i32; }
    int64_t asInt64() const { assert(isConstant()); return constant_.i64; }
    float asFloat32() const { assert(isConstant()); return constant_.f32; }
    double asFloat64() const { assert(isConstant()); return constant_.f64; }
    
    // =========================================================================
    // Auxiliary Data (for special opcodes)
    // =========================================================================
    
    void setAuxInt(int64_t v) { auxInt_ = v; }
    int64_t auxInt() const { return auxInt_; }
    
    // For Call: function index
    // For GetLocal/SetLocal: local index
    // For Branch targets: use dedicated methods
    
    // =========================================================================
    // Flags
    // =========================================================================
    
    enum Flag : uint32_t {
        Dead = 1 << 0,
        Visited = 1 << 1,
        InWorklist = 1 << 2,
    };
    
    bool hasFlag(Flag f) const { return (flags_ & f) != 0; }
    void setFlag(Flag f) { flags_ |= f; }
    void clearFlag(Flag f) { flags_ &= ~f; }
    void clearAllFlags() { flags_ = 0; }
    
    bool isDead() const { return hasFlag(Dead); }
    void markDead() { setFlag(Dead); }
    
    // =========================================================================
    // Replacement (for optimization)
    // =========================================================================
    
    void replaceAllUsesWith(Value* replacement) {
        assert(replacement != this);
        for (Value* user : users_) {
            for (uint32_t i = 0; i < user->numInputs(); ++i) {
                if (user->input(i) == this) {
                    user->inputs_[i] = replacement;
                    replacement->addUser(user);
                }
            }
        }
        users_.clear();
    }
    
private:
    Opcode op_;
    Type type_;
    uint32_t index_;
    BasicBlock* block_;
    uint32_t flags_;
    
    std::vector<Value*> inputs_;
    std::vector<Value*> users_;
    
    ConstantValue constant_;
    int64_t auxInt_ = 0;
};

// =============================================================================
// Helper: Create common values
// =============================================================================

inline Value* createConst32(uint32_t index, int32_t val) {
    auto* v = new Value(Opcode::Const32, Type::I32, index);
    v->setConstant(ConstantValue(val));
    return v;
}

inline Value* createConst64(uint32_t index, int64_t val) {
    auto* v = new Value(Opcode::Const64, Type::I64, index);
    v->setConstant(ConstantValue(val));
    return v;
}

inline Value* createConstF32(uint32_t index, float val) {
    auto* v = new Value(Opcode::ConstF32, Type::F32, index);
    v->setConstant(ConstantValue(val));
    return v;
}

inline Value* createConstF64(uint32_t index, double val) {
    auto* v = new Value(Opcode::ConstF64, Type::F64, index);
    v->setConstant(ConstantValue(val));
    return v;
}

} // namespace Zepra::DFG
