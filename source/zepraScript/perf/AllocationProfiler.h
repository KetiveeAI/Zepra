/**
 * @file AllocationProfiler.h
 * @brief Memory allocation profiling
 * 
 * Implements:
 * - Allocation tracking by type
 * - Allocation site recording
 * - Allocation rate monitoring
 * - Large allocation detection
 * 
 * For debugging memory-intensive applications
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>

namespace Zepra::Perf {

// =============================================================================
// Allocation Entry
// =============================================================================

struct AllocationEntry {
    void* address;
    size_t size;
    std::string type;
    std::chrono::steady_clock::time_point timestamp;
    
    // Allocation site
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber = 0;
    
    // Stack trace (optional)
    std::vector<std::string> stack;
    
    // GC metadata
    uint8_t generation = 0;     // 0 = nursery, 1+ = old gen
    bool isPromoted = false;
    bool isFreed = false;
};

// =============================================================================
// Allocation Site Stats
// =============================================================================

struct AllocationSiteStats {
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber;
    
    // Counts
    uint64_t totalAllocations = 0;
    uint64_t totalBytes = 0;
    uint64_t liveAllocations = 0;
    uint64_t liveBytes = 0;
    
    // Size distribution
    uint64_t smallAllocs = 0;   // < 256 bytes
    uint64_t mediumAllocs = 0;  // 256 bytes - 1KB
    uint64_t largeAllocs = 0;   // > 1KB
    
    // Type breakdown
    std::unordered_map<std::string, uint64_t> byType;
    
    double avgSize() const {
        return totalAllocations > 0 ? 
               static_cast<double>(totalBytes) / totalAllocations : 0;
    }
};

// =============================================================================
// Type Statistics
// =============================================================================

struct TypeAllocationStats {
    std::string typeName;
    uint64_t totalCount = 0;
    uint64_t totalBytes = 0;
    uint64_t liveCount = 0;
    uint64_t liveBytes = 0;
    
    // Size range
    size_t minSize = SIZE_MAX;
    size_t maxSize = 0;
    
    void recordAllocation(size_t size) {
        totalCount++;
        totalBytes += size;
        liveCount++;
        liveBytes += size;
        minSize = std::min(minSize, size);
        maxSize = std::max(maxSize, size);
    }
    
    void recordFree(size_t size) {
        liveCount--;
        liveBytes -= size;
    }
};

// =============================================================================
// Allocation Profiler
// =============================================================================

class AllocationProfiler {
public:
    AllocationProfiler() = default;
    
    // =========================================================================
    // Control
    // =========================================================================
    
    void start();
    void stop();
    bool isTracking() const { return tracking_.load(); }
    void reset();
    
    /**
     * @brief Enable stack trace recording (expensive)
     */
    void setRecordStackTraces(bool enable) { recordStacks_ = enable; }
    
    /**
     * @brief Set large allocation threshold
     */
    void setLargeAllocationThreshold(size_t bytes) { largeThreshold_ = bytes; }
    
    // =========================================================================
    // Allocation Recording
    // =========================================================================
    
    /**
     * @brief Record an allocation
     */
    void recordAllocation(void* address, 
                          size_t size,
                          const std::string& type,
                          const std::string& function = "",
                          const std::string& script = "",
                          uint32_t line = 0);
    
    /**
     * @brief Record a deallocation
     */
    void recordFree(void* address);
    
    /**
     * @brief Record object promotion (nursery -> old gen)
     */
    void recordPromotion(void* address);
    
    // =========================================================================
    // Queries
    // =========================================================================
    
    /**
     * @brief Get allocation sites sorted by total bytes
     */
    std::vector<AllocationSiteStats> getTopAllocationSites(size_t limit = 20) const;
    
    /**
     * @brief Get allocation stats by type
     */
    std::vector<TypeAllocationStats> getTypeStats() const;
    
    /**
     * @brief Get live allocations (not freed)
     */
    std::vector<AllocationEntry> getLiveAllocations() const;
    
    /**
     * @brief Get large allocations (> threshold)
     */
    std::vector<AllocationEntry> getLargeAllocations() const;
    
    /**
     * @brief Get recent allocations
     */
    std::vector<AllocationEntry> getRecentAllocations(size_t limit = 100) const;
    
    // =========================================================================
    // Allocation Rate
    // =========================================================================
    
    struct AllocationRate {
        double allocsPerSecond;
        double bytesPerSecond;
        std::chrono::milliseconds windowSize;
    };
    
    AllocationRate getAllocationRate() const;
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    struct Stats {
        uint64_t totalAllocations = 0;
        uint64_t totalBytes = 0;
        uint64_t peakLiveBytes = 0;
        uint64_t currentLiveBytes = 0;
        uint64_t largeAllocations = 0;
        uint64_t promotions = 0;
        
        // By generation
        uint64_t nurseryAllocations = 0;
        uint64_t oldGenAllocations = 0;
    };
    
    Stats getStats() const;
    
    // =========================================================================
    // Export
    // =========================================================================
    
    /**
     * @brief Export allocation timeline as JSON
     */
    std::string exportTimelineJSON() const;
    
    /**
     * @brief Export summary report
     */
    std::string exportReport() const;
    
private:
    mutable std::mutex mutex_;
    std::atomic<bool> tracking_{false};
    
    // Live allocations
    std::unordered_map<void*, AllocationEntry> liveAllocations_;
    
    // Statistics
    std::unordered_map<std::string, AllocationSiteStats> siteStats_;
    std::unordered_map<std::string, TypeAllocationStats> typeStats_;
    
    // Recent allocations for timeline
    std::vector<AllocationEntry> recentAllocations_;
    static constexpr size_t MAX_RECENT = 10000;
    
    // Configuration
    bool recordStacks_ = false;
    size_t largeThreshold_ = 1024 * 1024;  // 1MB
    
    // Timing
    std::chrono::steady_clock::time_point startTime_;
    uint64_t allocsInWindow_ = 0;
    uint64_t bytesInWindow_ = 0;
    std::chrono::steady_clock::time_point windowStart_;
    
    Stats stats_;
    
    std::string makeSiteKey(const std::string& func, const std::string& script, uint32_t line);
    std::vector<std::string> captureStack();
};

} // namespace Zepra::Perf
