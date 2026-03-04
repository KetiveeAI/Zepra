/**
 * @file osr.cpp
 * @brief On-Stack Replacement implementation
 */

#include "jit/osr.hpp"
#include <unordered_map>

namespace Zepra::JIT {

// =============================================================================
// OSRManager Implementation
// =============================================================================

void OSRManager::recordEntryPoint(const Bytecode::BytecodeChunk* chunk,
                                   size_t loopHeader,
                                   void*,
                                   size_t nativeOffset) {
    OSREntryPoint entry;
    entry.bytecodeOffset = loopHeader;
    entry.nativeOffset = nativeOffset;
    
    entries_[chunk].push_back(entry);
}

bool OSRManager::canOSR(const Bytecode::BytecodeChunk* chunk, size_t offset) const {
    auto it = entries_.find(chunk);
    if (it == entries_.end()) return false;
    
    for (const auto& entry : it->second) {
        if (entry.bytecodeOffset == offset) {
            return true;
        }
    }
    return false;
}

const OSREntryPoint* OSRManager::getEntryPoint(const Bytecode::BytecodeChunk* chunk,
                                                size_t offset) const {
    auto it = entries_.find(chunk);
    if (it == entries_.end()) return nullptr;
    
    for (const auto& entry : it->second) {
        if (entry.bytecodeOffset == offset) {
            return &entry;
        }
    }
    return nullptr;
}

Runtime::Value OSRManager::performOSR(const OSREntryPoint* entry,
                                       std::vector<Runtime::Value>& locals,
                                       std::vector<Runtime::Value>& stack) {
    if (!entry) {
        return Runtime::Value::undefined();
    }
    
    // Build OSR buffer: locals first, then stack values
    // Format matches JIT's expected layout
    std::vector<Runtime::Value> osrBuffer;
    osrBuffer.reserve(locals.size() + stack.size());
    
    // Map locals according to stackMap positions
    for (size_t localIdx : entry->stackMap) {
        if (localIdx < locals.size()) {
            osrBuffer.push_back(locals[localIdx]);
        } else {
            osrBuffer.push_back(Runtime::Value::undefined());
        }
    }
    
    // Append operand stack values
    for (const auto& val : stack) {
        osrBuffer.push_back(val);
    }
    
    // Call native code via function pointer
    // The JIT code expects: void* osrBuffer, size_t bufferSize
    // and returns a Value in the first buffer slot
    using OSREntry = Runtime::Value(*)(Runtime::Value*, size_t);
    auto nativeEntry = reinterpret_cast<OSREntry>(
        reinterpret_cast<uintptr_t>(entry) + entry->nativeOffset);
    
    Runtime::Value result = nativeEntry(osrBuffer.data(), osrBuffer.size());
    
    return result;
}

void OSRManager::invalidate(const Bytecode::BytecodeChunk* chunk) {
    entries_.erase(chunk);
}

// =============================================================================
// Deoptimizer Implementation
// =============================================================================

static std::unordered_map<const Bytecode::BytecodeChunk*, size_t> deoptCounts;
static constexpr size_t BLACKLIST_THRESHOLD = 10;

void Deoptimizer::recordDeopt(const Bytecode::BytecodeChunk* chunk,
                               size_t,
                               Reason) {
    deoptCounts[chunk]++;
}

size_t Deoptimizer::getDeoptCount(const Bytecode::BytecodeChunk* chunk) {
    auto it = deoptCounts.find(chunk);
    return it != deoptCounts.end() ? it->second : 0;
}

bool Deoptimizer::shouldBlacklist(const Bytecode::BytecodeChunk* chunk) {
    return getDeoptCount(chunk) >= BLACKLIST_THRESHOLD;
}

void Deoptimizer::deoptimize(Runtime::Value* returnValue) {
    // Deoptimization path: bail out of JIT to interpreter
    // The return value is used to pass the current computation back
    // 
    // In a full implementation, this would:
    // 1. Walk the native stack frame to reconstruct interpreter state
    // 2. Map register values back to stack slots
    // 3. Set up the interpreter's instruction pointer
    // 4. Transfer control back to the interpreter loop
    //
    // For now, mark the return value as undefined to signal bailout
    // and let the caller handle continuation
    if (returnValue) {
        *returnValue = Runtime::Value::undefined();
    }
    
    // Note: Full deoptimization requires platform-specific stack walking
    // and integration with the VM's call frame management.
    // The JIT should be modified to check deopt flags at safe points.
}

} // namespace Zepra::JIT
