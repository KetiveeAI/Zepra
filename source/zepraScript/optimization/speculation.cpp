/**
 * @file speculation.cpp
 * @brief Type speculation and type inference system for Zepra JIT compilers
 *
 * This subsystem is responsible for:
 * - Tracking observed types during interpreter or Baseline JIT execution.
 * - Making speculative assumptions about variable/register types.
 * - Providing a TypeOracle for DFG/FTL JIT compilers to query expected types.
 * - Managing deoptimization checkpoints when speculation fails.
 *
 * Ref: WebKit DFG/FTL Type Profiling & Speculation, V8 TurboFan TypeFeedback
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdio>
#include <algorithm>

namespace Zepra::Optimization {

// =============================================================================
// SpeculatedType (Bitmask representation)
// =============================================================================

using SpeculatedType = uint32_t;

namespace Speculation {
    constexpr SpeculatedType None            = 0;
    
    constexpr SpeculatedType Int32           = 1 << 0;
    constexpr SpeculatedType Double          = 1 << 1;
    constexpr SpeculatedType Number          = Int32 | Double;
    
    constexpr SpeculatedType Boolean         = 1 << 2;
    constexpr SpeculatedType String          = 1 << 3;
    constexpr SpeculatedType Symbol          = 1 << 4;
    constexpr SpeculatedType BigInt          = 1 << 5;
    
    constexpr SpeculatedType Null            = 1 << 6;
    constexpr SpeculatedType Undefined       = 1 << 7;
    constexpr SpeculatedType Nullish         = Null | Undefined;
    
    constexpr SpeculatedType Object          = 1 << 8;
    constexpr SpeculatedType Function        = 1 << 9;
    constexpr SpeculatedType Array           = 1 << 10;
    
    constexpr SpeculatedType Top             = 0xFFFFFFFF; // Any type

    // String representation for debugging
    std::string typeToString(SpeculatedType type) {
        if (type == None) return "None";
        if (type == Top) return "Top";
        
        std::string result;
        auto add = [&](const char* name) {
            if (!result.empty()) result += "|";
            result += name;
        };
        
        if (type & Int32) add("Int32");
        if (type & Double) add("Double");
        if (type & Boolean) add("Boolean");
        if (type & String) add("String");
        if (type & Symbol) add("Symbol");
        if (type & BigInt) add("BigInt");
        if (type & Null) add("Null");
        if (type & Undefined) add("Undefined");
        if ((type & Object) && !(type & Function) && !(type & Array)) add("Object");
        if (type & Function) add("Function");
        if (type & Array) add("Array");
        
        return result.empty() ? "Unknown" : result;
    }
}

// =============================================================================
// TypeObserver (Profiles values at runtime)
// =============================================================================

class TypeObserver {
public:
    TypeObserver() : observedType_(Speculation::None), sampleCount_(0) {}

    /**
     * Observe a runtime value to update the type profile.
     * In a real engine, this inspects the Value object. Here we simulate
     * the underlying type classification.
     */
    void observeType(SpeculatedType type) {
        observedType_ |= type;
        sampleCount_++;
    }

    SpeculatedType getObservedType() const { return observedType_; }
    uint32_t getSampleCount() const { return sampleCount_; }
    
    bool isMonomorphic() const {
        // Monomorphic if exactly one bit is set
        return observedType_ != 0 && (observedType_ & (observedType_ - 1)) == 0;
    }
    
    void reset() {
        observedType_ = Speculation::None;
        sampleCount_ = 0;
    }

private:
    SpeculatedType observedType_;
    uint32_t sampleCount_;
};

// =============================================================================
// ProfileManager (Aggregates profiles for a CodeBlock)
// =============================================================================

class ProfileManager {
public:
    ProfileManager(uint32_t numBytecodes) {
        bytecodeProfiles_.resize(numBytecodes);
    }

    TypeObserver& getProfileForBytecode(uint32_t offset) {
        if (offset >= bytecodeProfiles_.size()) {
            bytecodeProfiles_.resize(offset + 1);
        }
        return bytecodeProfiles_[offset];
    }
    
    void dump() const {
        printf("ProfileManager stats:\n");
        for (size_t i = 0; i < bytecodeProfiles_.size(); ++i) {
            const auto& profile = bytecodeProfiles_[i];
            if (profile.getSampleCount() > 0) {
                printf("  [BC %3zu] Type: %-20s (Samples: %u)\n", 
                       i, Speculation::typeToString(profile.getObservedType()).c_str(),
                       profile.getSampleCount());
            }
        }
    }

private:
    std::vector<TypeObserver> bytecodeProfiles_;
};

// =============================================================================
// Deoptimization System
// =============================================================================

enum class DeoptReason : uint8_t {
    NotInt32,
    NotDouble,
    NotNumber,
    NotObject,
    NotString,
    OutOfBounds,
    HiddenClassMismatch,
    DivisionByZero,
    Overflow
};

const char* deoptReasonToString(DeoptReason reason) {
    switch (reason) {
        case DeoptReason::NotInt32: return "NotInt32";
        case DeoptReason::NotDouble: return "NotDouble";
        case DeoptReason::NotNumber: return "NotNumber";
        case DeoptReason::NotObject: return "NotObject";
        case DeoptReason::NotString: return "NotString";
        case DeoptReason::OutOfBounds: return "OutOfBounds";
        case DeoptReason::HiddenClassMismatch: return "HiddenClassMismatch";
        case DeoptReason::DivisionByZero: return "DivisionByZero";
        case DeoptReason::Overflow: return "Overflow";
        default: return "Unknown";
    }
}

/**
 * A Deoptimization point maps a machine code location back to the
 * interpreter state (bytecode offset, registers) so execution can
 * resume cleanly if a speculative assumption is violated.
 */
struct DeoptimizePoint {
    uint32_t bytecodeOffset;
    DeoptReason reason;
    uint32_t machineCodeOffset;
    
    // In a real system, we'd also store register mappings:
    // std::vector<RegisterMapping> registerState;
};

// =============================================================================
// Type Oracle (Query interface for JIT compiler)
// =============================================================================

/**
 * The TypeOracle is used by the DFG/FTL JIT to decide when to insert 
 * speculation checks and unbox values.
 */
class TypeOracle {
public:
    explicit TypeOracle(ProfileManager* profiles) : profiles_(profiles) {}

    SpeculatedType getSpeculationFor(uint32_t bytecodeOffset) const {
        if (!profiles_) return Speculation::Top;
        const auto& profile = const_cast<ProfileManager*>(profiles_)->getProfileForBytecode(bytecodeOffset);
        
        // If we haven't seen it execute enough, default to Top (no speculation)
        if (profile.getSampleCount() < 10) return Speculation::Top;
        
        return profile.getObservedType();
    }
    
    bool isLikelyInt32(uint32_t bytecodeOffset) const {
        return getSpeculationFor(bytecodeOffset) == Speculation::Int32;
    }
    
    bool isLikelyNumber(uint32_t bytecodeOffset) const {
        SpeculatedType type = getSpeculationFor(bytecodeOffset);
        return (type & Speculation::Number) == type && type != Speculation::None;
    }

private:
    ProfileManager* profiles_;
};

// =============================================================================
// Data-Flow Analysis (DFA) Type Inference
// =============================================================================

/**
 * Basic block representation for type inference.
 */
struct BasicBlock {
    uint32_t id;
    std::vector<uint32_t> predecessors;
    std::vector<uint32_t> successors;
    
    // Abstract state of locals at the start and end of the block
    std::unordered_map<uint32_t, SpeculatedType> stateIn;
    std::unordered_map<uint32_t, SpeculatedType> stateOut;
};

/**
 * A simple forward data-flow analyzer that computes reachable types 
 * for variables across basic blocks.
 */
class TypeInferenceEngine {
public:
    TypeInferenceEngine() = default;

    void addBlock(const BasicBlock& block) {
        blocks_[block.id] = block;
    }
    
    void analyze() {
        bool changed = true;
        uint32_t iterationCount = 0;
        
        while (changed && iterationCount < 100) {
            changed = false;
            iterationCount++;
            
            for (auto& pair : blocks_) {
                BasicBlock& block = pair.second;
                
                // IN = union of OUT of all predecessors
                std::unordered_map<uint32_t, SpeculatedType> newIn;
                for (uint32_t predId : block.predecessors) {
                    const auto& predOut = blocks_[predId].stateOut;
                    for (const auto& var : predOut) {
                        newIn[var.first] |= var.second;
                    }
                }
                
                if (newIn != block.stateIn) {
                    block.stateIn = newIn;
                    changed = true;
                }
                
                // OUT = IN (modified by block's operations)
                // In a full implementation, we would simulate the effect of each instruction in the block
                // Here we just pass IN to OUT for unmodified variables
                std::unordered_map<uint32_t, SpeculatedType> newOut = block.stateIn;
                
                // Simulate some operations adding specific types (mock logic)
                for (auto& var : block.stateOut) {
                    // Retain any explicitly inferred types from the block's instructions
                    newOut[var.first] |= var.second; 
                }
                
                if (newOut != block.stateOut) {
                    block.stateOut = newOut;
                    changed = true;
                }
            }
        }
        
        printf("Type inference completed in %u iterations.\n", iterationCount);
    }
    
    void dump() const {
        printf("Type Inference Results:\n");
        for (const auto& pair : blocks_) {
            const BasicBlock& block = pair.second;
            printf("  Block %u:\n", block.id);
            printf("    IN:\n");
            for (const auto& var : block.stateIn) {
                printf("      Var %u: %s\n", var.first, Speculation::typeToString(var.second).c_str());
            }
            printf("    OUT:\n");
            for (const auto& var : block.stateOut) {
                printf("      Var %u: %s\n", var.first, Speculation::typeToString(var.second).c_str());
            }
        }
    }

private:
   std::unordered_map<uint32_t, BasicBlock> blocks_;
};

// =============================================================================
// Public Subsystem API
// =============================================================================

class SpeculationSubsystem {
public:
    static SpeculationSubsystem& instance() {
        static SpeculationSubsystem inst;
        return inst;
    }

    ProfileManager* createProfileManager(uint32_t codeBlockId, uint32_t numBytecodes) {
        auto pm = std::make_unique<ProfileManager>(numBytecodes);
        ProfileManager* ptr = pm.get();
        managers_[codeBlockId] = std::move(pm);
        return ptr;
    }
    
    TypeOracle createOracle(uint32_t codeBlockId) {
        auto it = managers_.find(codeBlockId);
        if (it != managers_.end()) {
            return TypeOracle(it->second.get());
        }
        return TypeOracle(nullptr);
    }
    
    void clearProfiles(uint32_t codeBlockId) {
        managers_.erase(codeBlockId);
    }

private:
    SpeculationSubsystem() = default;
    
    std::unordered_map<uint32_t, std::unique_ptr<ProfileManager>> managers_;
};

} // namespace Zepra::Optimization
