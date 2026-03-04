#pragma once

/**
 * @file osr.hpp
 * @brief On-Stack Replacement for JIT compilation
 */

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <vector>
#include <unordered_map>

namespace Zepra::Bytecode { struct BytecodeChunk; }

namespace Zepra::JIT {

/**
 * @brief OSR entry point information
 */
struct OSREntryPoint {
    size_t bytecodeOffset;    // Where in bytecode to enter
    size_t nativeOffset;      // Where in native code to jump
    std::vector<size_t> stackMap;  // Local variable positions
};

/**
 * @brief On-Stack Replacement manager
 * 
 * Handles transition from interpreted to JIT-compiled code
 * mid-execution (typically in hot loops).
 */
class OSRManager {
public:
    /**
     * @brief Record an OSR entry point for a function
     */
    void recordEntryPoint(const Bytecode::BytecodeChunk* chunk,
                          size_t loopHeader,
                          void* nativeCode,
                          size_t nativeOffset);
    
    /**
     * @brief Check if OSR is available at current position
     */
    bool canOSR(const Bytecode::BytecodeChunk* chunk, size_t offset) const;
    
    /**
     * @brief Get OSR entry point
     */
    const OSREntryPoint* getEntryPoint(const Bytecode::BytecodeChunk* chunk,
                                       size_t offset) const;
    
    /**
     * @brief Perform OSR: transfer execution to JIT code
     * @param locals Current local variables
     * @param stack Current operand stack
     * @return Result from JIT execution
     */
    Runtime::Value performOSR(const OSREntryPoint* entry,
                              std::vector<Runtime::Value>& locals,
                              std::vector<Runtime::Value>& stack);
    
    /**
     * @brief Clear all OSR data for a function
     */
    void invalidate(const Bytecode::BytecodeChunk* chunk);
    
private:
    std::unordered_map<const Bytecode::BytecodeChunk*,
                       std::vector<OSREntryPoint>> entries_;
};

/**
 * @brief Deoptimization handler
 * 
 * Handles bailout from JIT-compiled code back to interpreter
 * when assumptions are violated.
 */
class Deoptimizer {
public:
    /**
     * @brief Reasons for deoptimization
     */
    enum class Reason {
        TypeMismatch,      // Type speculation failed
        Overflow,          // Integer overflow
        DivisionByZero,
        NullReference,
        DebuggerBreakpoint,
        ProfileRedirect,   // Re-profile needed
        Unknown
    };
    
    /**
     * @brief Record deoptimization event
     */
    static void recordDeopt(const Bytecode::BytecodeChunk* chunk,
                            size_t offset,
                            Reason reason);
    
    /**
     * @brief Get deopt count for function
     */
    static size_t getDeoptCount(const Bytecode::BytecodeChunk* chunk);
    
    /**
     * @brief Check if function should be blacklisted from JIT
     */
    static bool shouldBlacklist(const Bytecode::BytecodeChunk* chunk);
    
    /**
     * @brief Perform deoptimization
     */
    static void deoptimize(Runtime::Value* returnValue);
};

} // namespace Zepra::JIT
