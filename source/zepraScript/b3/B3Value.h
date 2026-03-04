/**
 * @file B3Value.h
 * @brief B3 SSA Value
 * 
 * Low-level SSA value representation for B3 backend IR.
 */

#pragma once

#include "B3Opcode.h"
#include <vector>
#include <cstdint>

namespace Zepra::B3 {

class BasicBlock;

// Memory access properties
enum class MemoryKind : uint8_t {
    None,
    HeapRead,
    HeapWrite,
    StackRead,
    StackWrite,
};

class Value {
public:
    Value(Opcode op, Type type, uint32_t index)
        : opcode_(op), type_(type), index_(index) {}
    
    Opcode opcode() const { return opcode_; }
    Type type() const { return type_; }
    uint32_t index() const { return index_; }
    BasicBlock* block() const { return block_; }
    
    void setBlock(BasicBlock* b) { block_ = b; }
    
    // Inputs (operands)
    uint32_t numInputs() const { return static_cast<uint32_t>(inputs_.size()); }
    Value* input(uint32_t i) const { return inputs_[i]; }
    void addInput(Value* v) { inputs_.push_back(v); }
    void setInput(uint32_t i, Value* v) { inputs_[i] = v; }
    const std::vector<Value*>& inputs() const { return inputs_; }
    
    // Def-use chain
    const std::vector<Value*>& users() const { return users_; }
    void addUser(Value* v) { users_.push_back(v); }
    void removeUser(Value* v) {
        for (auto it = users_.begin(); it != users_.end(); ++it) {
            if (*it == v) { users_.erase(it); return; }
        }
    }
    
    // Replace all uses of this value with another
    void replaceAllUsesWith(Value* replacement) {
        for (Value* user : users_) {
            for (uint32_t i = 0; i < user->numInputs(); ++i) {
                if (user->input(i) == this) {
                    user->setInput(i, replacement);
                }
            }
            replacement->addUser(user);
        }
        users_.clear();
    }
    
    // Constant accessors
    void setConstInt32(int32_t v) { constData_.i32 = v; }
    void setConstInt64(int64_t v) { constData_.i64 = v; }
    void setConstFloat(float v) { constData_.f32 = v; }
    void setConstDouble(double v) { constData_.f64 = v; }
    
    int32_t constInt32() const { return constData_.i32; }
    int64_t constInt64() const { return constData_.i64; }
    float constFloat() const { return constData_.f32; }
    double constDouble() const { return constData_.f64; }
    
    bool isConstant() const { return Zepra::B3::isConstant(opcode_); }
    
    // Memory properties
    MemoryKind memoryKind() const { return memoryKind_; }
    void setMemoryKind(MemoryKind k) { memoryKind_ = k; }
    
    // Dead code elimination
    bool isDead() const { return dead_; }
    void markDead() { dead_ = true; }
    
    // Auxiliary data (for calls, branches)
    void setAuxInt(int64_t v) { auxInt_ = v; }
    int64_t auxInt() const { return auxInt_; }
    
private:
    Opcode opcode_;
    Type type_;
    uint32_t index_;
    BasicBlock* block_ = nullptr;
    
    std::vector<Value*> inputs_;
    std::vector<Value*> users_;
    
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    } constData_ = {0};
    
    MemoryKind memoryKind_ = MemoryKind::None;
    int64_t auxInt_ = 0;
    bool dead_ = false;
};

} // namespace Zepra::B3
