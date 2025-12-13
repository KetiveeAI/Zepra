#pragma once

/**
 * @file vm.hpp
 * @brief Virtual Machine for bytecode execution
 */

#include "../config.hpp"
#include "value.hpp"
#include "environment.hpp"
#include "../bytecode/opcode.hpp"
#include <vector>
#include <memory>
#include <stack>
#include <unordered_map>

namespace Zepra::Runtime {

// Forward declarations
class Context;
class Object;
class Function;
class GCHeap;
class RuntimeUpvalue;

}  // namespace Zepra::Runtime

namespace Zepra::GC {
class Heap;
}

namespace Zepra::Bytecode {
class BytecodeChunk;
}

namespace Zepra::Runtime {

/**
 * @brief Call frame for function execution
 */
struct CallFrame {
    Function* function = nullptr;
    const uint8_t* ip = nullptr;        // Instruction pointer
    size_t stackBase = 0;               // Base of this frame in operand stack
    Environment* environment = nullptr; // Current scope
    Value thisValue;                    // 'this' binding
};

/**
 * @brief Execution result
 */
struct ExecutionResult {
    enum class Status {
        Success,
        Exception,
        Error
    };
    
    Status status = Status::Success;
    Value value;
    std::string error;
};

/**
 * @brief VM call stack frame
 */
struct VMCallFrame {
    Function* function = nullptr;
    size_t returnAddress = 0;
    size_t slotBase = 0;
    Value thisValue;
    const Bytecode::BytecodeChunk* savedChunk = nullptr;  // Caller's chunk for iterative calls
};

/**
 * @brief Exception handler for try/catch blocks
 */
struct ExceptionHandler {
    size_t catchAddress = 0;    // Jump target for catch block
    size_t finallyAddress = 0;  // Jump target for finally block
    size_t stackLevel = 0;      // Stack size when try began
    size_t callStackLevel = 0;  // Call stack size when try began
};

/**
 * @brief Virtual Machine for executing JavaScript bytecode
 */
class VM {
public:
    explicit VM(Context* context);
    ~VM();
    
    /**
     * @brief Execute a bytecode chunk
     */
    ExecutionResult execute(const Bytecode::BytecodeChunk* chunk);
    
    /**
     * @brief Call a function
     */
    Value call(Function* fn, Value thisValue, const std::vector<Value>& args);
    
    /**
     * @brief Construct an object
     */
    Value construct(Function* fn, const std::vector<Value>& args);
    
    /**
     * @brief Get the current context
     */
    Context* context() const { return context_; }
    
    /**
     * @brief Stack operations
     */
    void push(Value value);
    Value pop();
    Value peek(size_t distance = 0) const;
    void popN(size_t count);
    
    /**
     * @brief Get stack size
     */
    size_t stackSize() const { return stack_.size(); }
    
    /**
     * @brief Get current call depth (for debugger)
     */
    size_t getCallDepth() const { return nativeDepth_ + heapDepth_; }
    
    /**
     * @brief Set a global variable
     */
    void setGlobal(const std::string& name, Value value) { globals_[name] = value; }
    
    /**
     * @brief Get a global variable
     */
    Value getGlobal(const std::string& name) const {
        auto it = globals_.find(name);
        return it != globals_.end() ? it->second : Value::undefined();
    }
    
    /**
     * @brief Set current module path for import resolution
     */
    void setModulePath(const std::string& path) { currentModulePath_ = path; }
    const std::string& currentModulePath() const { return currentModulePath_; }

    
private:
    // Bytecode execution
    void run();
    void dispatch(Zepra::Bytecode::Opcode op);
    
    // Read helpers
    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    
    // Local variable access
    Value getLocal(size_t slot);
    void setLocal(size_t slot, Value value);
    
    Context* context_;
    
    // Current bytecode chunk
    const Bytecode::BytecodeChunk* chunk_ = nullptr;
    size_t ip_ = 0;
    
    // Operand stack
    std::vector<Value> stack_;
    
    // ==========================================================================
    // HYBRID TWO-STACK ARCHITECTURE
    // Fast path: Native C stack for 99% of calls (zero allocation)
    // Slow path: Heap stack for deep recursion (grows as needed)
    // ==========================================================================
    
    // Stack configuration constants
    static constexpr size_t NATIVE_STACK_SIZE = 512;      // Frames on C stack
    static constexpr size_t NATIVE_STACK_THRESHOLD = 384; // 75% - switch point
    static constexpr size_t HEAP_STACK_PREALLOC = 1024;   // Pre-allocate frames
    static constexpr size_t HEAP_STACK_MAX = 65536;       // Max 1.8MB heap
    
    // Native stack (FAST PATH - zero allocation)
    VMCallFrame nativeStack_[NATIVE_STACK_SIZE];
    size_t nativeDepth_ = 0;
    
    // Heap stack (SLOW PATH - for deep recursion)
    std::vector<VMCallFrame> heapStack_;
    size_t heapDepth_ = 0;
    
    // Helper: Get current call depth (total)
    size_t callDepth() const { return nativeDepth_ + heapDepth_; }
    
    // Helper: Should use heap stack?
    bool shouldUseHeapStack() const { return nativeDepth_ >= NATIVE_STACK_THRESHOLD; }
    
    // Helper: Get current frame
    VMCallFrame& currentFrame() {
        if (heapDepth_ > 0) return heapStack_[heapDepth_ - 1];
        if (nativeDepth_ > 0) return nativeStack_[nativeDepth_ - 1];
        static VMCallFrame empty;
        return empty;
    }
    
    // Helper: Push call frame (auto-switches)
    void pushCallFrame(const VMCallFrame& frame);
    
    // Helper: Pop call frame (auto-switches)  
    VMCallFrame popCallFrame();
    
    // Helper: Cleanup excess heap after returning to shallow
    void shrinkHeapIfNeeded();
    
    // Global variables
    std::unordered_map<std::string, Value> globals_;
    
    // Open upvalues (linked list, ordered by stack slot)
    RuntimeUpvalue* openUpvalues_ = nullptr;
    
    // Exception handling
    std::vector<ExceptionHandler> exceptionHandlers_;
    Value exceptionValue_;
    bool hasException_ = false;
    
    // Helper to capture/reuse upvalue
    RuntimeUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);
    
    // Module loading
    std::string currentModulePath_;
};

} // namespace Zepra::Runtime

