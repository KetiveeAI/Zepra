/**
 * @file WasmTrapHandler.h
 * @brief WebAssembly Trap Handling & Metadata
 * 
 * Maps machine code offsets to trap reasons.
 * Used by signal handlers to report correct WASM errors.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <mutex>
#include "wasm/WasmSignalHandlers.h"

namespace Zepra::Wasm {

/**
 * @brief Trap site metadata
 */
struct TrapSite {
    uint32_t offset;  // Offset from code start
    WasmTrapException::TrapReason reason;
};

/**
 * @brief Maps PC offsets to trap reasons for a code block
 */
class TrapHandlerMap {
public:
    void addTrapSite(uint32_t offset, WasmTrapException::TrapReason reason) {
        sites_.push_back({offset, reason});
    }
    
    // Call after adding all sites
    void sort() {
        std::sort(sites_.begin(), sites_.end(), 
            [](const TrapSite& a, const TrapSite& b) {
                return a.offset < b.offset;
            });
    }
    
    // Find trap reason for a PC offset
    // Returns true if found, false otherwise
    bool findReason(uint32_t offset, WasmTrapException::TrapReason& outReason) const {
        auto it = std::lower_bound(sites_.begin(), sites_.end(), offset,
            [](const TrapSite& site, uint32_t off) {
                return site.offset < off;
            });
            
        // Check exact match or range? 
        // Typically trap instruction is 1-4 bytes. We look for nearest preceding or exact.
        // For now, assume exact offset match (or within small range of faulting instr)
        if (it != sites_.end() && it->offset == offset) {
            outReason = it->reason;
            return true;
        }
        return false;
    }
    
private:
    std::vector<TrapSite> sites_;
};

/**
 * @brief Global registry of trap maps
 */
class GlobalTrapRegistry {
public:
    static GlobalTrapRegistry& instance() {
        static GlobalTrapRegistry registry;
        return registry;
    }
    
    void registerMap(uintptr_t codeStart, size_t codeSize, TrapHandlerMap map) {
        std::lock_guard<std::mutex> lock(mutex_);
        regions_.push_back({codeStart, codeSize, std::move(map)});
    }
    
    bool lookupTrap(uintptr_t pc, WasmTrapException::TrapReason& outReason) {
        // Can't use mutex in signal handler!
        // In production, use lock-free reading (e.g. atomic head pointer or RCU)
        // For this implementation, we assume regions don't change during execution of that code
        
        for (const auto& region : regions_) {
            if (pc >= region.start && pc < region.start + region.size) {
                uint32_t offset = static_cast<uint32_t>(pc - region.start);
                return region.map.findReason(offset, outReason);
            }
        }
        return false;
    }
    
private:
    struct Region {
        uintptr_t start;
        size_t size;
        TrapHandlerMap map;
    };
    
    std::vector<Region> regions_;
    std::mutex mutex_;
};

} // namespace Zepra::Wasm
