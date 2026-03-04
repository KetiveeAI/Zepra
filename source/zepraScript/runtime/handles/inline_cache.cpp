/**
 * @file inline_cache.cpp
 * @brief Inline caching implementation
 */

#include "runtime/handles/inline_cache.hpp"
#include <algorithm>

namespace Zepra::Runtime {

// InlineCache implementation

bool InlineCache::lookup(uint32_t shapeId, uint32_t& offset) {
    // Check each entry for a match
    for (size_t i = 0; i < MAX_ENTRIES; ++i) {
        if (entries_[i].valid && entries_[i].shapeId == shapeId) {
            // Cache hit!
            offset = entries_[i].propertyOffset;
            entries_[i].hitCount++;
            hits_++;
            return true;
        }
    }
    
    // Cache miss
    misses_++;
    return false;
}

void InlineCache::update(uint32_t shapeId, uint32_t offset) {
    // First, check if we already have this shape (update in place)
    for (size_t i = 0; i < MAX_ENTRIES; ++i) {
        if (entries_[i].valid && entries_[i].shapeId == shapeId) {
            entries_[i].propertyOffset = offset;
            return;
        }
    }
    
    // Find an empty slot
    for (size_t i = 0; i < MAX_ENTRIES; ++i) {
        if (!entries_[i].valid) {
            entries_[i].shapeId = shapeId;
            entries_[i].propertyOffset = offset;
            entries_[i].hitCount = 0;
            entries_[i].valid = true;
            return;
        }
    }
    
    // Cache full - use round-robin eviction (simpler than LRU, similar performance)
    entries_[nextEvict_].shapeId = shapeId;
    entries_[nextEvict_].propertyOffset = offset;
    entries_[nextEvict_].hitCount = 0;
    entries_[nextEvict_].valid = true;
    nextEvict_ = (nextEvict_ + 1) % MAX_ENTRIES;
}

void InlineCache::clear() {
    for (size_t i = 0; i < MAX_ENTRIES; ++i) {
        entries_[i].invalidate();
    }
    nextEvict_ = 0;
    // Don't reset statistics - useful for profiling
}

float InlineCache::hitRate() const {
    uint32_t total = hits_ + misses_;
    if (total == 0) return 0.0f;
    return static_cast<float>(hits_) / static_cast<float>(total);
}

// ICManager implementation

InlineCache* ICManager::getIC(size_t bytecodeOffset) {
    // First, look for existing IC at this offset
    for (size_t i = 0; i < usedSlots_; ++i) {
        if (slots_[i].used && slots_[i].offset == bytecodeOffset) {
            return &slots_[i].cache;
        }
    }
    
    // Create new IC if we have room
    if (usedSlots_ < MAX_IC_SITES) {
        slots_[usedSlots_].offset = bytecodeOffset;
        slots_[usedSlots_].used = true;
        return &slots_[usedSlots_++].cache;
    }
    
    // Too many IC sites - return nullptr (caller will use slow path)
    return nullptr;
}

void ICManager::invalidateAll() {
    for (size_t i = 0; i < usedSlots_; ++i) {
        slots_[i].cache.clear();
    }
}

size_t ICManager::memoryUsage() const {
    // Each used slot contributes sizeof(ICSlot)
    // ICSlot = size_t (8) + InlineCache (~80) + bool (1) ≈ 96 bytes
    return usedSlots_ * sizeof(ICSlot);
}

float ICManager::globalHitRate() const {
    uint32_t totalHits = 0;
    uint32_t totalMisses = 0;
    
    for (size_t i = 0; i < usedSlots_; ++i) {
        if (slots_[i].used) {
            totalHits += slots_[i].cache.hits();
            totalMisses += slots_[i].cache.misses();
        }
    }
    
    uint32_t total = totalHits + totalMisses;
    if (total == 0) return 0.0f;
    return static_cast<float>(totalHits) / static_cast<float>(total);
}

} // namespace Zepra::Runtime
