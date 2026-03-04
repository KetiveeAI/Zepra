#pragma once

/**
 * @file inline_cache.hpp
 * @brief Inline caching for fast property access
 * 
 * Monomophic inline cache (IC) for property lookups.
 * Fixed-size cache with minimal memory footprint.
 */

#include "config.hpp"
#include "runtime/objects/value.hpp"
#include <cstdint>
#include <string>

namespace Zepra::Runtime {

class Object;

/**
 * @brief Single inline cache entry
 * 
 * Caches the shape/structure of an object and the property offset.
 * On cache hit, skip the full property lookup.
 */
struct ICEntry {
    uint32_t shapeId = 0;     // Object shape identifier
    uint32_t propertyOffset = 0; // Cached property offset
    uint16_t hitCount = 0;    // For LRU/statistics
    bool valid = false;       // Entry is populated
    
    void invalidate() {
        shapeId = 0;
        propertyOffset = 0;
        hitCount = 0;
        valid = false;
    }
};

/**
 * @brief Inline cache for a call site
 * 
 * Small fixed-size cache (4 entries max) per property access site.
 * Memory: 16 bytes per entry × 4 = 64 bytes per IC
 */
class InlineCache {
public:
    static constexpr size_t MAX_ENTRIES = 4;
    
    /**
     * @brief Try to get cached property offset for an object
     * @param shapeId The object's structure identifier
     * @param offset Output: cached property offset
     * @return true if cache hit, false if miss
     */
    bool lookup(uint32_t shapeId, uint32_t& offset);
    
    /**
     * @brief Update cache with new mapping
     * @param shapeId Object structure identifier
     * @param offset Property offset to cache
     */
    void update(uint32_t shapeId, uint32_t offset);
    
    /**
     * @brief Invalidate all entries
     */
    void clear();
    
    /**
     * @brief Get hit rate for profiling
     */
    float hitRate() const;
    
    // Statistics
    uint32_t hits() const { return hits_; }
    uint32_t misses() const { return misses_; }
    
private:
    ICEntry entries_[MAX_ENTRIES] = {};
    uint32_t hits_ = 0;
    uint32_t misses_ = 0;
    size_t nextEvict_ = 0;  // Round-robin eviction
};

/**
 * @brief Global IC manager for all property access sites
 * 
 * Maps bytecode offsets to inline caches.
 * Memory-efficient: only allocates ICs for hot paths.
 */
class ICManager {
public:
    /**
     * @brief Get or create IC for a bytecode offset
     */
    InlineCache* getIC(size_t bytecodeOffset);
    
    /**
     * @brief Clear all caches (on GC or shape invalidation)
     */
    void invalidateAll();
    
    /**
     * @brief Get total memory used by all ICs
     */
    size_t memoryUsage() const;
    
    /**
     * @brief Get hit rate across all ICs
     */
    float globalHitRate() const;
    
private:
    // Simple array-based storage for ICs
    // Key: bytecode offset, Value: inline cache
    static constexpr size_t MAX_IC_SITES = 256;
    
    struct ICSlot {
        size_t offset = 0;
        InlineCache cache;
        bool used = false;
    };
    
    ICSlot slots_[MAX_IC_SITES] = {};
    size_t usedSlots_ = 0;
};

} // namespace Zepra::Runtime
