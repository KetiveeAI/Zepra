/**
 * @file Interpreter.h
 * @brief Bytecode interpreter core
 * 
 * Implements:
 * - Dispatch loop
 * - Instruction handlers
 * - Exception handling
 * - Profiling hooks
 * 
 * Based on threaded interpreter design
 */

#pragma once

#include "bytecode/OpcodeReference.h"
#include "StackFrame.h"
#include "heap/handle.hpp"
#include <functional>

namespace Zepra::VM {

// Forward declarations
class VirtualMachine;
class Function;
class Environment;

// =============================================================================
// Interpreter State
// =============================================================================

enum class InterpreterState : uint8_t {
    Idle,
    Running,
    Paused,         // Debugger pause
    Exception,
    Finished
};

// =============================================================================
// Interpreter Configuration
// =============================================================================

struct InterpreterConfig {
    bool enableProfiling = false;
    bool enableTracing = false;
    size_t maxStackSize = 1024 * 1024;  // 1MB
    size_t tierUpThreshold = 1000;      // Calls before JIT
};

// =============================================================================
// Profiling Data
// =============================================================================

struct ExecutionProfile {
    size_t instructionsExecuted = 0;
    size_t callsExecuted = 0;
    size_t branchesTaken = 0;
    size_t branchesNotTaken = 0;
    std::unordered_map<uint32_t, size_t> hotLoops;  // bytecode offset -> count
    std::unordered_map<uint32_t, size_t> hotCalls;  // function id -> count
};

// =============================================================================
// Interpreter
// =============================================================================

class Interpreter {
public:
    explicit Interpreter(VirtualMachine& vm);
    ~Interpreter() = default;
    
    // Execute bytecode
    Runtime::Value execute(Function* function, 
                           Runtime::Value thisValue,
                           const std::vector<Runtime::Value>& args);
    
    // Execute with existing frame
    Runtime::Value executeFrame(CallFrame& frame);
    
    // State
    InterpreterState state() const { return state_; }
    bool isRunning() const { return state_ == InterpreterState::Running; }
    
    // Current execution
    CallFrame* currentFrame() { return callStack_.empty() ? nullptr : &callStack_.top(); }
    size_t callDepth() const { return callStack_.depth(); }
    
    // Exception handling
    Runtime::Value lastException() const { return exception_; }
    bool hasException() const { return state_ == InterpreterState::Exception; }
    void clearException() { exception_ = Runtime::Value::undefined(); }
    
    // Profiling
    const ExecutionProfile& profile() const { return profile_; }
    void resetProfile() { profile_ = ExecutionProfile(); }
    
    // Configuration
    void configure(const InterpreterConfig& config) { config_ = config; }
    
    // Hooks for JIT tier-up
    using TierUpCallback = std::function<void(Function*, size_t)>;
    void setTierUpCallback(TierUpCallback cb) { tierUpCallback_ = std::move(cb); }
    
private:
    VirtualMachine& vm_;
    CallStack callStack_;
    InterpreterState state_ = InterpreterState::Idle;
    Runtime::Value exception_;
    InterpreterConfig config_;
    ExecutionProfile profile_;
    TierUpCallback tierUpCallback_;
    
    // Instruction dispatch
    Runtime::Value dispatch(CallFrame& frame);
    
    // Instruction handlers (selected subset)
    void handleLoad(CallFrame& frame, Bytecode::Opcode op);
    void handleStore(CallFrame& frame, Bytecode::Opcode op);
    void handleArithmetic(CallFrame& frame, Bytecode::Opcode op);
    void handleBitwise(CallFrame& frame, Bytecode::Opcode op);
    void handleComparison(CallFrame& frame, Bytecode::Opcode op);
    void handleControlFlow(CallFrame& frame, Bytecode::Opcode op);
    void handleCall(CallFrame& frame, Bytecode::Opcode op);
    void handleProperty(CallFrame& frame, Bytecode::Opcode op);
    void handleException(CallFrame& frame, Bytecode::Opcode op);
    
    // Stack operations
    void push(CallFrame& frame, Runtime::Value value);
    Runtime::Value pop(CallFrame& frame);
    Runtime::Value peek(CallFrame& frame, size_t depth = 0);
    
    // Bytecode reading
    uint8_t readByte(CallFrame& frame);
    uint16_t readShort(CallFrame& frame);
    uint32_t readInt(CallFrame& frame);
    
    // Exception handling
    bool unwindToHandler(CallFrame& frame, Runtime::Value exception);
    void throwException(Runtime::Value exception);
    
    // Tier-up check
    void checkTierUp(Function* function);
};

// =============================================================================
// Threaded Dispatch Table (for direct threading)
// =============================================================================

#ifdef ZEPRA_USE_COMPUTED_GOTO

/**
 * @brief Direct threaded interpreter using computed goto
 */
class ThreadedInterpreter {
public:
    explicit ThreadedInterpreter(VirtualMachine& vm);
    
    Runtime::Value execute(Function* function,
                           Runtime::Value thisValue,
                           const std::vector<Runtime::Value>& args);
    
private:
    VirtualMachine& vm_;
    
    // Dispatch table: opcode -> label address
    static void* dispatchTable_[256];
    static bool tableInitialized_;
    
    static void initDispatchTable();
};

#endif // ZEPRA_USE_COMPUTED_GOTO

} // namespace Zepra::VM
