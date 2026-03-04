/**
 * @file WasmStackManager.h
 * @brief WebAssembly Stack Overflow Protection
 * 
 * Manages stack limits and guard pages for safe execution.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <thread>

namespace Zepra::Wasm {

class StackManager {
public:
    static StackManager& instance() {
        static StackManager manager;
        return manager;
    }
    
    // Get stack limit for current thread
    uintptr_t getCurrentStackLimit() {
        // Simple implementation: return thread-local limit
        // In real impl, would read from TCB or VM context
        return threadLimit_;
    }
    
    void setCurrentStackLimit(uintptr_t limit) {
        threadLimit_ = limit;
    }
    
    // Initialize stack for a thread
    void initThreadStack(size_t stackSize) {
        // Calculate stack bounds
        // Set guard pages at bottom
        // threadLimit_ = ...
    }
    
private:
    static thread_local uintptr_t threadLimit_;
};

// Thread-local storage definition
// In C++17 inline variables are better, but we use this pattern compatibility
// Definition needs to be in .cpp usually, but for header-only simple style:
// (Actually we need .cpp or just rely on the compiler)

} // namespace Zepra::Wasm
