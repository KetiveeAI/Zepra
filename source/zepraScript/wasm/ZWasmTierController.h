/**
 * @file ZWasmTierController.h
 * @brief ZepraScript WASM Tiering Controller
 * 
 * Manages tier-up from interpreter to baseline JIT to optimizing JIT.
 * Part of ZepraScript's independent WASM implementation.
 */

#pragma once

#include "WasmConstants.h"  // TODO: rename to ZWasmConstants.h
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>
#include <cstdint>

namespace Zepra::Wasm {

// =============================================================================
// Tier-Up Trigger Reasons
// =============================================================================

enum class TierUpTrigger : uint8_t {
    DontTrigger,
    StartCompilation,
    CompilationDone,
};

// =============================================================================
// Compilation Result
// =============================================================================

enum class CompilationResult : uint8_t {
    Successful,
    Failed,
    Deferred,
    Invalidated
};

// =============================================================================
// OSR Entry Data (On-Stack Replacement)
// =============================================================================

struct OSREntryData {
    uint32_t loopIndex;
    uint32_t functionIndex;
    std::vector<uint8_t> stackMap;
    void* compiledCode;
    
    OSREntryData(uint32_t loop, uint32_t func)
        : loopIndex(loop)
        , functionIndex(func)
        , compiledCode(nullptr) {}
};

// =============================================================================
// Function Tiering Info
// =============================================================================

struct ZFuncHotness {
    CompilationTier currentTier = CompilationTier::None;
    CompilationStatus baselineStatus = CompilationStatus::NotCompiled;
    CompilationStatus optimizedStatus = CompilationStatus::NotCompiled;
    
    std::atomic<int32_t> callCount{0};
    std::atomic<int32_t> loopCount{0};
    std::atomic<bool> isCompiling{false};
    
    // Thresholds
    static constexpr int32_t InterpreterToBaselineThreshold = 10;
    static constexpr int32_t BaselineToOptimizedThreshold = 10000;
    static constexpr int32_t LoopOSRThreshold = 1000;
    
    void incrementCallCount() {
        callCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    void incrementLoopCount() {
        loopCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    bool shouldTierUpToBaseline() const {
        return currentTier == CompilationTier::None 
            && callCount.load(std::memory_order_relaxed) >= InterpreterToBaselineThreshold
            && !isCompiling.load(std::memory_order_relaxed);
    }
    
    bool shouldTierUpToOptimized() const {
        return currentTier == CompilationTier::Baseline
            && (callCount.load(std::memory_order_relaxed) >= BaselineToOptimizedThreshold
                || loopCount.load(std::memory_order_relaxed) >= LoopOSRThreshold)
            && !isCompiling.load(std::memory_order_relaxed);
    }
    
    bool shouldOSREnterLoop() const {
        return currentTier == CompilationTier::Baseline
            && loopCount.load(std::memory_order_relaxed) >= LoopOSRThreshold
            && optimizedStatus == CompilationStatus::Compiled;
    }
};

// =============================================================================
// Tier-Up Count Manager
// =============================================================================

class ZTierController {
public:
    ZTierController() = default;
    ~ZTierController() = default;
    
    // Thresholds (configurable)
    static int32_t loopIncrement() { return 1; }
    static int32_t functionEntryIncrement() { return 1; }
    
    // Function entry hook
    void onFunctionEntry(uint32_t funcIdx) {
        ensureFuncInfo(funcIdx).incrementCallCount();
    }
    
    // Loop backedge hook
    void onLoopBackedge(uint32_t funcIdx, uint32_t loopIdx) {
        ensureFuncInfo(funcIdx).incrementLoopCount();
    }
    
    // Check if tier-up is needed
    bool checkTierUp(uint32_t funcIdx) {
        auto* info = getFuncInfo(funcIdx);
        if (!info) return false;
        return info->shouldTierUpToBaseline() || info->shouldTierUpToOptimized();
    }
    
    // Get current tier
    CompilationTier currentTier(uint32_t funcIdx) const {
        auto* info = getFuncInfo(funcIdx);
        if (!info) return CompilationTier::None;
        return info->currentTier;
    }
    
    // Set tier after compilation
    void setTier(uint32_t funcIdx, CompilationTier tier) {
        auto& info = ensureFuncInfo(funcIdx);
        info.currentTier = tier;
        info.isCompiling.store(false, std::memory_order_release);
    }
    
    // Mark as compiling
    bool tryStartCompilation(uint32_t funcIdx) {
        auto& info = ensureFuncInfo(funcIdx);
        bool expected = false;
        return info.isCompiling.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel);
    }
    
    // Handle compilation result
    void setCompilationResult(uint32_t funcIdx, CompilationResult result) {
        auto& info = ensureFuncInfo(funcIdx);
        
        switch (result) {
        case CompilationResult::Successful:
            if (info.currentTier == CompilationTier::None) {
                info.baselineStatus = CompilationStatus::Compiled;
                info.currentTier = CompilationTier::Baseline;
            } else {
                info.optimizedStatus = CompilationStatus::Compiled;
                info.currentTier = CompilationTier::Optimized;
            }
            break;
            
        case CompilationResult::Failed:
            if (info.currentTier == CompilationTier::None) {
                info.baselineStatus = CompilationStatus::Failed;
            } else {
                info.optimizedStatus = CompilationStatus::Failed;
            }
            // Defer future attempts
            info.callCount.store(0, std::memory_order_relaxed);
            break;
            
        case CompilationResult::Deferred:
            // Will retry later
            break;
            
        case CompilationResult::Invalidated:
            // Reset and retry
            info.callCount.store(0, std::memory_order_relaxed);
            break;
        }
        
        info.isCompiling.store(false, std::memory_order_release);
    }
    
    // OSR entry management
    OSREntryData& addOSREntryData(uint32_t funcIdx, uint32_t loopIdx) {
        std::lock_guard<std::mutex> lock(osrLock_);
        osrEntryData_.push_back(std::make_unique<OSREntryData>(loopIdx, funcIdx));
        return *osrEntryData_.back();
    }
    
    OSREntryData* getOSREntryData(uint32_t funcIdx, uint32_t loopIdx) {
        std::lock_guard<std::mutex> lock(osrLock_);
        for (auto& entry : osrEntryData_) {
            if (entry->functionIndex == funcIdx && entry->loopIndex == loopIdx) {
                return entry.get();
            }
        }
        return nullptr;
    }
    
    // Get trigger reason for a loop
    TierUpTrigger osrEntryTrigger(uint32_t funcIdx) {
        auto* info = getFuncInfo(funcIdx);
        if (!info) return TierUpTrigger::DontTrigger;
        
        if (info->optimizedStatus == CompilationStatus::Compiled) {
            return TierUpTrigger::CompilationDone;
        }
        if (info->shouldTierUpToOptimized() && !info->isCompiling.load()) {
            return TierUpTrigger::StartCompilation;
        }
        return TierUpTrigger::DontTrigger;
    }
    
private:
    ZFuncHotness& ensureFuncInfo(uint32_t funcIdx) {
        std::lock_guard<std::mutex> lock(infoLock_);
        while (funcIdx >= funcInfo_.size()) {
            funcInfo_.push_back(std::make_unique<ZFuncHotness>());
        }
        return *funcInfo_[funcIdx];
    }
    
    ZFuncHotness* getFuncInfo(uint32_t funcIdx) const {
        if (funcIdx >= funcInfo_.size()) return nullptr;
        return funcInfo_[funcIdx].get();
    }
    
    std::vector<std::unique_ptr<ZFuncHotness>> funcInfo_;
    std::vector<std::unique_ptr<OSREntryData>> osrEntryData_;
    std::mutex infoLock_;
    std::mutex osrLock_;
};

// =============================================================================
// Compilation Options
// =============================================================================

struct CompilationOptions {
    bool enableBaseline = true;
    bool enableOptimized = true;
    bool enableOSR = true;
    
    int32_t baselineThreshold = ZFuncHotness::InterpreterToBaselineThreshold;
    int32_t optimizedThreshold = ZFuncHotness::BaselineToOptimizedThreshold;
    int32_t osrThreshold = ZFuncHotness::LoopOSRThreshold;
    
    bool parallelCompilation = true;
    uint32_t maxCompilationThreads = 4;
};

} // namespace Zepra::Wasm
