/**
 * @file SpeculativeInliner.h
 * @brief Speculative Function Inlining
 * 
 * JSC FTL-inspired patterns:
 * - Call site profiling
 * - Budget-based inlining decisions
 * - Inline depth limits
 * - Bailout on speculation failure
 */

#pragma once

#include "../dfg/DFGGraph.h"
#include "../dfg/DFGValue.h"
#include "type_profiler.hpp"

#include <vector>
#include <unordered_set>

namespace Zepra::JIT {

// =============================================================================
// Call Link Info (Cached Call Targets)
// =============================================================================

/**
 * @brief Tracks call site targets for inlining
 */
class CallLinkInfo {
public:
    explicit CallLinkInfo(uint32_t bytecodeOffset)
        : bytecodeOffset_(bytecodeOffset) {}
    
    uint32_t bytecodeOffset() const { return bytecodeOffset_; }
    
    // Record observed callee
    void recordCallee(uintptr_t calleeId) {
        callCounts_[calleeId]++;
        totalCalls_++;
    }
    
    // Get most frequent callee
    uintptr_t primaryCallee() const {
        uintptr_t best = 0;
        uint32_t bestCount = 0;
        
        for (const auto& [id, count] : callCounts_) {
            if (count > bestCount) {
                best = id;
                bestCount = count;
            }
        }
        return best;
    }
    
    // Check if monomorphic (single target)
    bool isMonomorphic() const {
        return callCounts_.size() == 1;
    }
    
    // Check if polymorphic but inlinable
    bool isPolymorphic() const {
        return callCounts_.size() > 1 && callCounts_.size() <= 4;
    }
    
    // Hit rate for primary target
    double primaryHitRate() const {
        if (totalCalls_ == 0) return 0;
        auto it = callCounts_.find(primaryCallee());
        if (it == callCounts_.end()) return 0;
        return static_cast<double>(it->second) / totalCalls_;
    }
    
    uint32_t totalCalls() const { return totalCalls_; }
    
private:
    uint32_t bytecodeOffset_;
    std::unordered_map<uintptr_t, uint32_t> callCounts_;
    uint32_t totalCalls_ = 0;
};

// =============================================================================
// Inline Candidate
// =============================================================================

/**
 * @brief Inline decision result
 */
enum class InlineDecision {
    Inline,           // Should inline
    NoInline,         // Don't inline (heuristic)
    TooLarge,         // Callee too large
    TooDeep,          // Inline depth exceeded
    Recursive,        // Would create recursion
    NotMonomorphic,   // Call site not stable
    Native,           // Can't inline native
    Unknown           // Unknown function
};

/**
 * @brief Candidate for inlining
 */
struct InlineCandidate {
    CallLinkInfo* callInfo;
    uintptr_t calleeId;
    uint32_t calleeSize;      // Bytecode size
    uint32_t currentDepth;
    InlineDecision decision;
    
    // Cost/benefit
    uint32_t expectedBenefit; // Based on call frequency
    uint32_t cost;            // Bytecode to inline
};

// =============================================================================
// Inline Budget
// =============================================================================

/**
 * @brief Controls inlining limits
 */
struct InlineBudget {
    // Per-site limits
    uint32_t maxInlineSize = 100;       // Max bytecode ops
    uint32_t maxInlineDepth = 5;        // Max nesting
    
    // Per-function limits
    uint32_t maxTotalInlined = 500;     // Total inlined ops
    uint32_t maxInlineSites = 10;       // Max sites to inline
    
    // Thresholds
    uint32_t minCallCount = 10;         // Min calls before inline
    double minHitRate = 0.9;            // 90% monomorphic
    
    // Current usage
    uint32_t usedOps = 0;
    uint32_t usedSites = 0;
    
    bool canInline(const InlineCandidate& candidate) const {
        if (usedOps + candidate.calleeSize > maxTotalInlined) return false;
        if (usedSites >= maxInlineSites) return false;
        if (candidate.currentDepth >= maxInlineDepth) return false;
        if (candidate.calleeSize > maxInlineSize) return false;
        return true;
    }
    
    void consume(const InlineCandidate& candidate) {
        usedOps += candidate.calleeSize;
        usedSites++;
    }
};

// =============================================================================
// Speculative Inliner
// =============================================================================

/**
 * @brief Performs speculative function inlining
 */
class SpeculativeInliner {
public:
    explicit SpeculativeInliner(DFG::Graph* graph)
        : graph_(graph) {}
    
    // Run inlining pass
    bool run() {
        collectCandidates();
        rankCandidates();
        
        for (auto& candidate : candidates_) {
            if (!budget_.canInline(candidate)) {
                candidate.decision = InlineDecision::TooLarge;
                continue;
            }
            
            if (shouldInline(candidate)) {
                if (performInline(candidate)) {
                    budget_.consume(candidate);
                    inlinedFunctions_.insert(candidate.calleeId);
                }
            }
        }
        
        return !inlinedFunctions_.empty();
    }
    
    const std::unordered_set<uintptr_t>& inlinedFunctions() const {
        return inlinedFunctions_;
    }
    
private:
    void collectCandidates() {
        // Would scan DFG for Call nodes
        // For each call, check if we have CallLinkInfo
    }
    
    void rankCandidates() {
        // Sort by expected benefit (call count * size reduction)
        std::sort(candidates_.begin(), candidates_.end(),
            [](const InlineCandidate& a, const InlineCandidate& b) {
                return a.expectedBenefit > b.expectedBenefit;
            });
    }
    
    bool shouldInline(InlineCandidate& candidate) {
        // Check call site stability
        if (!candidate.callInfo->isMonomorphic() &&
            candidate.callInfo->primaryHitRate() < budget_.minHitRate) {
            candidate.decision = InlineDecision::NotMonomorphic;
            return false;
        }
        
        // Check call count
        if (candidate.callInfo->totalCalls() < budget_.minCallCount) {
            candidate.decision = InlineDecision::NoInline;
            return false;
        }
        
        // Check for recursion
        if (inlinedFunctions_.count(candidate.calleeId)) {
            candidate.decision = InlineDecision::Recursive;
            return false;
        }
        
        candidate.decision = InlineDecision::Inline;
        return true;
    }
    
    bool performInline(const InlineCandidate& candidate) {
        // 1. Insert guard for callee identity
        // 2. Copy callee's DFG nodes
        // 3. Replace call with inlined body
        // 4. Add bailout path for guard failure
        
        (void)candidate;
        return true;
    }
    
    DFG::Graph* graph_;
    InlineBudget budget_;
    std::vector<InlineCandidate> candidates_;
    std::unordered_set<uintptr_t> inlinedFunctions_;
};

// =============================================================================
// Inline Statistics
// =============================================================================

struct InlineStats {
    uint32_t candidatesConsidered = 0;
    uint32_t inlined = 0;
    uint32_t rejectedSize = 0;
    uint32_t rejectedDepth = 0;
    uint32_t rejectedPolymorphic = 0;
    uint32_t rejectedRecursive = 0;
    uint32_t bytecodeInlined = 0;
};

} // namespace Zepra::JIT
