/**
 * @file StackFrame.h
 * @brief Virtual machine stack frame structure
 * 
 * Implements:
 * - Interpreter frame layout
 * - Inlined frame tracking
 * - Stack walking
 * - Frame inspection for debugging
 * 
 * Based on V8/JSC frame layouts
 */

#pragma once

#include "runtime/objects/value.hpp"
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace Zepra::VM {

// Forward declarations
class Script;
class Function;
class Environment;

// =============================================================================
// Frame Types
// =============================================================================

enum class FrameType : uint8_t {
    Interpreter,        // Interpreter execution frame
    BaselineJIT,        // Baseline JIT frame
    OptimizedJIT,       // DFG/optimized JIT frame
    Builtin,            // Native builtin call
    Construct,          // new expression frame
    InlinedFrame,       // Inlined call (virtual)
    Stub,               // JIT stub frame
    Entry               // VM entry frame
};

// =============================================================================
// Frame Layout
// =============================================================================

/**
 * Stack frame layout (grows down):
 * 
 * High addresses
 * +-------------------+
 * | Caller's frame    |
 * +-------------------+
 * | Return address    |  <- RBP + 8
 * +-------------------+
 * | Saved RBP         |  <- RBP (frame pointer)
 * +-------------------+
 * | Frame marker      |  <- RBP - 8 (FrameType, flags)
 * +-------------------+
 * | Function          |  <- RBP - 16
 * +-------------------+
 * | Bytecode offset   |  <- RBP - 24 (for interpreter)
 * +-------------------+
 * | Argument count    |  <- RBP - 32
 * +-------------------+
 * | This value        |  <- RBP - 40
 * +-------------------+
 * | Local 0           |  <- RBP - 48
 * +-------------------+
 * | Local 1           |
 * +-------------------+
 * | ...               |
 * +-------------------+
 * | Operand stack     |  <- RSP
 * +-------------------+
 * Low addresses
 */

struct FrameSlots {
    static constexpr int ReturnAddress = 8;
    static constexpr int SavedFP = 0;
    static constexpr int FrameMarker = -8;
    static constexpr int Function = -16;
    static constexpr int BytecodeOffset = -24;
    static constexpr int ArgCount = -32;
    static constexpr int ThisValue = -40;
    static constexpr int LocalsStart = -48;
};

// =============================================================================
// Stack Frame
// =============================================================================

class StackFrame {
public:
    StackFrame() = default;
    explicit StackFrame(void* fp) : fp_(static_cast<intptr_t*>(fp)) {}
    
    bool isValid() const { return fp_ != nullptr; }
    
    // Frame navigation
    StackFrame caller() const {
        return StackFrame(reinterpret_cast<void*>(fp_[0]));
    }
    
    void* returnAddress() const {
        return reinterpret_cast<void*>(fp_[1]);
    }
    
    // Frame type
    FrameType type() const {
        return static_cast<FrameType>(readSlot(FrameSlots::FrameMarker) & 0xFF);
    }
    
    // Frame content
    Function* function() const {
        return reinterpret_cast<Function*>(readSlot(FrameSlots::Function));
    }
    
    size_t bytecodeOffset() const {
        return static_cast<size_t>(readSlot(FrameSlots::BytecodeOffset));
    }
    
    size_t argumentCount() const {
        return static_cast<size_t>(readSlot(FrameSlots::ArgCount));
    }
    
    Runtime::Value thisValue() const {
        return Runtime::Value::number(static_cast<double>(readSlot(FrameSlots::ThisValue)));
    }
    
    // Local access
    Runtime::Value getLocal(size_t index) const {
        size_t offset = FrameSlots::LocalsStart - index * 8;
        return Runtime::Value::number(static_cast<double>(readSlot(offset)));
    }
    
    void setLocal(size_t index, Runtime::Value value) {
        size_t offset = FrameSlots::LocalsStart - index * 8;
        writeSlot(offset, value.asNumber());
    }
    
    // Raw frame pointer
    void* framePointer() const { return fp_; }
    
    // Check frame type
    bool isInterpreter() const { return type() == FrameType::Interpreter; }
    bool isJIT() const { 
        auto t = type();
        return t == FrameType::BaselineJIT || t == FrameType::OptimizedJIT;
    }
    bool isNative() const { return type() == FrameType::Builtin; }
    
private:
    intptr_t* fp_ = nullptr;
    
    intptr_t readSlot(int offset) const {
        return *reinterpret_cast<intptr_t*>(
            reinterpret_cast<char*>(fp_) + offset);
    }
    
    void writeSlot(int offset, intptr_t value) {
        *reinterpret_cast<intptr_t*>(
            reinterpret_cast<char*>(fp_) + offset) = value;
    }
};

// =============================================================================
// Stack Walker
// =============================================================================

class StackWalker {
public:
    explicit StackWalker(StackFrame topFrame) : current_(topFrame) {}
    
    bool hasMore() const { return current_.isValid(); }
    
    StackFrame current() const { return current_; }
    
    void advance() {
        if (current_.isValid()) {
            current_ = current_.caller();
        }
    }
    
    // Collect all frames
    std::vector<StackFrame> collectFrames() {
        std::vector<StackFrame> frames;
        while (hasMore()) {
            frames.push_back(current());
            advance();
        }
        return frames;
    }
    
    // Find frame by type
    StackFrame findFrame(FrameType type) {
        while (hasMore()) {
            if (current().type() == type) {
                return current();
            }
            advance();
        }
        return StackFrame();
    }
    
private:
    StackFrame current_;
};

// =============================================================================
// Call Frame (high-level representation)
// =============================================================================

struct CallFrame {
    Function* function = nullptr;
    Runtime::Value* locals = nullptr;
    Runtime::Value* stack = nullptr;
    const uint8_t* ip = nullptr;        // Instruction pointer
    Runtime::Value thisValue;
    Environment* environment = nullptr;
    size_t argCount = 0;
    bool isTailCall = false;
    bool isConstructor = false;
    
    // Source location
    uint32_t sourceOffset = 0;
    uint32_t lineNumber = 0;
};

// =============================================================================
// Call Stack
// =============================================================================

class CallStack {
public:
    static constexpr size_t MAX_DEPTH = 10000;
    
    CallStack() { frames_.reserve(256); }
    
    void push(CallFrame frame) {
        if (frames_.size() >= MAX_DEPTH) {
            throw std::runtime_error("Maximum call stack size exceeded");
        }
        frames_.push_back(std::move(frame));
    }
    
    CallFrame pop() {
        if (frames_.empty()) {
            throw std::runtime_error("Call stack underflow");
        }
        CallFrame frame = std::move(frames_.back());
        frames_.pop_back();
        return frame;
    }
    
    CallFrame& top() { return frames_.back(); }
    const CallFrame& top() const { return frames_.back(); }
    
    size_t depth() const { return frames_.size(); }
    bool empty() const { return frames_.empty(); }
    
    // Stack trace
    std::vector<std::string> getStackTrace() const;
    
private:
    std::vector<CallFrame> frames_;
};

} // namespace Zepra::VM
