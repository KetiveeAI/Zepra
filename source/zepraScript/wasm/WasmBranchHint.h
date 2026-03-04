/**
 * @file WasmBranchHint.h
 * @brief WebAssembly Branch Hinting Proposal
 * 
 * Implements branch prediction hints:
 * - @likely/@unlikely annotations
 * - JIT code layout optimization
 */

#pragma once

#include <cstdint>

namespace Zepra::Wasm {

// =============================================================================
// Branch Hint Types
// =============================================================================

enum class BranchHint : uint8_t {
    None = 0,
    Likely = 1,     // Branch expected to be taken
    Unlikely = 2    // Branch expected to be not taken
};

/**
 * @brief Branch hint annotation
 */
struct BranchHintEntry {
    uint32_t funcIdx;      // Function index
    uint32_t branchOffset; // Offset of branch instruction
    BranchHint hint;       // Hint type
};

/**
 * @brief Branch hints section (custom section)
 */
class BranchHintSection {
public:
    static constexpr const char* SECTION_NAME = "metadata.code.branch_hint";
    
    void addHint(uint32_t funcIdx, uint32_t offset, BranchHint hint) {
        hints_.push_back({funcIdx, offset, hint});
    }
    
    BranchHint getHint(uint32_t funcIdx, uint32_t offset) const {
        for (const auto& h : hints_) {
            if (h.funcIdx == funcIdx && h.branchOffset == offset) {
                return h.hint;
            }
        }
        return BranchHint::None;
    }
    
    const std::vector<BranchHintEntry>& hints() const { return hints_; }
    
    // Parse from custom section bytes
    void parse(const uint8_t* data, size_t size) {
        size_t offset = 0;
        while (offset < size) {
            uint32_t funcIdx = readLEB128(data, offset);
            uint32_t count = readLEB128(data, offset);
            
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t branchOffset = readLEB128(data, offset);
                uint8_t hintVal = data[offset++];
                BranchHint hint = static_cast<BranchHint>(hintVal);
                hints_.push_back({funcIdx, branchOffset, hint});
            }
        }
    }
    
private:
    uint32_t readLEB128(const uint8_t* data, size_t& offset) {
        uint32_t result = 0;
        uint32_t shift = 0;
        while (true) {
            uint8_t byte = data[offset++];
            result |= (byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }
    
    std::vector<BranchHintEntry> hints_;
};

// =============================================================================
// JIT Integration
// =============================================================================

/**
 * @brief Apply hints to code generation
 */
class BranchHintOptimizer {
public:
    void setHints(const BranchHintSection* section) {
        hints_ = section;
    }
    
    // Get hint for current branch
    BranchHint currentHint(uint32_t funcIdx, uint32_t offset) const {
        return hints_ ? hints_->getHint(funcIdx, offset) : BranchHint::None;
    }
    
    // Code layout recommendation
    bool shouldInvertBranch(BranchHint hint) const {
        // If unlikely, invert so fall-through is hot path
        return hint == BranchHint::Unlikely;
    }
    
    // Prefetch hint for CPU
    bool shouldPrefetch(BranchHint hint) const {
        return hint == BranchHint::Likely;
    }
    
private:
    const BranchHintSection* hints_ = nullptr;
};

} // namespace Zepra::Wasm
