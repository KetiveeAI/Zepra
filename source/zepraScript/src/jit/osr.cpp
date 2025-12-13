/**
 * @file osr.cpp
 * @brief On-Stack Replacement implementation
 */

#include "zeprascript/jit/osr.hpp"
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

Runtime::Value OSRManager::performOSR(const OSREntryPoint*,
                                       std::vector<Runtime::Value>&,
                                       std::vector<Runtime::Value>&) {
    // TODO: Actually transfer to native code
    // This requires setting up registers and jumping
    return Runtime::Value::undefined();
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

void Deoptimizer::deoptimize(Runtime::Value*) {
    // TODO: Reconstruct interpreter state from native stack
    // and transfer back to interpreter
}

} // namespace Zepra::JIT
