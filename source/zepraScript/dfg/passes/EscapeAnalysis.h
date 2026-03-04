/**
 * @file EscapeAnalysis.h
 * @brief Escape Analysis and Scalar Replacement
 * 
 * Eliminates heap allocations for non-escaping objects:
 * - Analyze escape paths through DFG
 * - Replace object fields with stack slots
 * - Works with Arrays, Objects, Closures
 */

#pragma once

#include "../DFGGraph.h"
#include "../DFGValue.h"

#include <unordered_map>
#include <unordered_set>

namespace Zepra::DFG {

// =============================================================================
// Escape State
// =============================================================================

enum class EscapeState : uint8_t {
    Unknown,        // Not yet analyzed
    NoEscape,       // Definitely doesn't escape
    ArgEscape,      // Escapes through call argument
    GlobalEscape,   // Escapes to global/heap
    MayEscape       // Conservatively assume escape
};

/**
 * @brief Escape info for an allocation
 */
struct EscapeInfo {
    EscapeState state = EscapeState::Unknown;
    Value* allocation = nullptr;
    
    // Escape reasons (for debugging)
    std::vector<std::string> escapeReasons;
    
    // Fields accessed (for scalar replacement)
    std::unordered_set<uint32_t> accessedFields;
    
    bool canReplace() const {
        return state == EscapeState::NoEscape;
    }
};

// =============================================================================
// Escape Analyzer
// =============================================================================

/**
 * @brief Analyzes which allocations escape
 */
class EscapeAnalyzer {
public:
    explicit EscapeAnalyzer(Graph* graph) : graph_(graph) {}
    
    // Run analysis
    void analyze() {
        collectAllocations();
        propagateEscapes();
    }
    
    // Check if value escapes
    EscapeState escapeState(Value* v) const {
        auto it = escapeInfo_.find(v);
        if (it == escapeInfo_.end()) return EscapeState::MayEscape;
        return it->second.state;
    }
    
    // Get all non-escaping allocations
    std::vector<Value*> nonEscapingAllocations() const {
        std::vector<Value*> result;
        for (const auto& [alloc, info] : escapeInfo_) {
            if (info.state == EscapeState::NoEscape) {
                result.push_back(alloc);
            }
        }
        return result;
    }
    
    const EscapeInfo* getInfo(Value* v) const {
        auto it = escapeInfo_.find(v);
        return it != escapeInfo_.end() ? &it->second : nullptr;
    }
    
private:
    void collectAllocations() {
        graph_->forEachValue([this](Value* v) {
            if (isAllocation(v)) {
                escapeInfo_[v] = EscapeInfo{EscapeState::Unknown, v};
            }
        });
    }
    
    bool isAllocation(Value* v) const {
        switch (v->opcode()) {
            case Opcode::NewObject:
            case Opcode::NewArray:
            case Opcode::CreateClosure:
            case Opcode::NewArrayBuffer:
                return true;
            default:
                return false;
        }
    }
    
    void propagateEscapes() {
        // Fixed-point iteration
        bool changed = true;
        while (changed) {
            changed = false;
            
            for (auto& [alloc, info] : escapeInfo_) {
                if (info.state == EscapeState::GlobalEscape) continue;
                
                EscapeState newState = analyzeUses(alloc, info);
                if (newState != info.state) {
                    info.state = newState;
                    changed = true;
                }
            }
        }
        
        // Mark remaining Unknown as NoEscape
        for (auto& [_, info] : escapeInfo_) {
            if (info.state == EscapeState::Unknown) {
                info.state = EscapeState::NoEscape;
            }
        }
    }
    
    EscapeState analyzeUses(Value* alloc, EscapeInfo& info) {
        EscapeState worst = EscapeState::NoEscape;
        
        for (Value* use : alloc->uses()) {
            EscapeState useState = analyzeUse(alloc, use, info);
            if (useState > worst) {
                worst = useState;
            }
            if (worst == EscapeState::GlobalEscape) break;
        }
        
        return worst;
    }
    
    EscapeState analyzeUse(Value* alloc, Value* use, EscapeInfo& info) {
        switch (use->opcode()) {
            // Safe: field access (record field)
            case Opcode::GetInlineProperty:
            case Opcode::SetInlineProperty:
            case Opcode::GetOutOfLineProperty:
            case Opcode::SetOutOfLineProperty:
                if (use->child(0) == alloc) {
                    info.accessedFields.insert(use->constantAsU32());
                    return EscapeState::NoEscape;
                }
                // Value escapes as property value
                info.escapeReasons.push_back("stored as property");
                return EscapeState::GlobalEscape;
                
            // Safe: array access
            case Opcode::GetByIndex:
            case Opcode::SetByIndex:
                if (use->child(0) == alloc) {
                    return EscapeState::NoEscape;
                }
                info.escapeReasons.push_back("stored in array");
                return EscapeState::GlobalEscape;
                
            // Safe: length check
            case Opcode::GetArrayLength:
                return EscapeState::NoEscape;
                
            // Escape: passed to call
            case Opcode::Call:
            case Opcode::Construct:
                info.escapeReasons.push_back("passed to call");
                return EscapeState::ArgEscape;
                
            // Escape: returned
            case Opcode::Return:
                info.escapeReasons.push_back("returned from function");
                return EscapeState::GlobalEscape;
                
            // Escape: stored to global
            case Opcode::PutGlobalVar:
                info.escapeReasons.push_back("stored to global");
                return EscapeState::GlobalEscape;
                
            // Conservative: unknown use
            default:
                info.escapeReasons.push_back("unknown use");
                return EscapeState::MayEscape;
        }
    }
    
    Graph* graph_;
    std::unordered_map<Value*, EscapeInfo> escapeInfo_;
};

// =============================================================================
// Scalar Replacer
// =============================================================================

/**
 * @brief Replaces non-escaping objects with stack slots
 */
class ScalarReplacer {
public:
    ScalarReplacer(Graph* graph, EscapeAnalyzer* analyzer)
        : graph_(graph), analyzer_(analyzer) {}
    
    // Run replacement
    bool run() {
        auto allocations = analyzer_->nonEscapingAllocations();
        
        for (Value* alloc : allocations) {
            if (replaceAllocation(alloc)) {
                replacedCount_++;
            }
        }
        
        return replacedCount_ > 0;
    }
    
    size_t replacedCount() const { return replacedCount_; }
    
private:
    bool replaceAllocation(Value* alloc) {
        const EscapeInfo* info = analyzer_->getInfo(alloc);
        if (!info) return false;
        
        // Create stack slots for each field
        std::unordered_map<uint32_t, Value*> fieldSlots;
        for (uint32_t field : info->accessedFields) {
            auto* slot = graph_->createValue(Opcode::StackSlot, ValueType::Any);
            slot->setConstant(field);
            fieldSlots[field] = slot;
        }
        
        // Replace uses
        std::vector<Value*> usesToReplace(alloc->uses().begin(), alloc->uses().end());
        for (Value* use : usesToReplace) {
            replaceUse(alloc, use, fieldSlots);
        }
        
        // Remove allocation
        graph_->removeValue(alloc);
        
        return true;
    }
    
    void replaceUse(Value* alloc, Value* use,
                    const std::unordered_map<uint32_t, Value*>& fieldSlots) {
        switch (use->opcode()) {
            case Opcode::GetInlineProperty:
            case Opcode::GetOutOfLineProperty: {
                uint32_t field = use->constantAsU32();
                auto it = fieldSlots.find(field);
                if (it != fieldSlots.end()) {
                    use->replaceWithLoad(it->second);
                }
                break;
            }
            
            case Opcode::SetInlineProperty: 
            case Opcode::SetOutOfLineProperty: {
                uint32_t field = use->constantAsU32();
                auto it = fieldSlots.find(field);
                if (it != fieldSlots.end()) {
                    use->replaceWithStore(it->second, use->child(1));
                }
                break;
            }
            
            case Opcode::GetArrayLength:
                // Replace with known length if constant
                use->replaceWithConstant(0);  // Would track actual length
                break;
                
            default:
                break;
        }
    }
    
    Graph* graph_;
    EscapeAnalyzer* analyzer_;
    size_t replacedCount_ = 0;
};

// =============================================================================
// Combined Pass
// =============================================================================

/**
 * @brief Runs escape analysis + scalar replacement
 */
class EscapeAnalysisPass {
public:
    static bool run(Graph* graph) {
        EscapeAnalyzer analyzer(graph);
        analyzer.analyze();
        
        ScalarReplacer replacer(graph, &analyzer);
        return replacer.run();
    }
};

} // namespace Zepra::DFG
