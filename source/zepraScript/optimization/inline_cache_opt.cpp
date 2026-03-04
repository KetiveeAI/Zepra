/**
 * @file inline_cache_opt.cpp
 * @brief Polymorphic Inline Cache for property access optimization
 *
 * Inline caches (ICs) accelerate property lookups by caching the
 * hidden class → property offset mapping at each access site.
 *
 * States:
 *   - Uninitialized: First access, no cache
 *   - Monomorphic: One hidden class cached (most common, fastest)
 *   - Polymorphic: 2-4 hidden classes cached
 *   - Megamorphic: Too many shapes, fall back to hash lookup
 *
 * Ref: V8 IC system, SpiderMonkey CacheIR, JSC StructureStubInfo
 */

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <cstdio>

namespace Zepra::Optimization {

// =============================================================================
// IC State
// =============================================================================

enum class ICState : uint8_t {
    Uninitialized,
    Monomorphic,
    Polymorphic,
    Megamorphic,
    Generic       // Gave up, always use slow path
};

static const char* icStateName(ICState s) {
    switch (s) {
        case ICState::Uninitialized: return "uninit";
        case ICState::Monomorphic:   return "mono";
        case ICState::Polymorphic:   return "poly";
        case ICState::Megamorphic:   return "mega";
        case ICState::Generic:       return "generic";
    }
    return "?";
}

// =============================================================================
// IC Entry (cached lookup result)
// =============================================================================

struct ICEntry {
    uint32_t hiddenClassId = 0; // Hidden class this entry matched
    int32_t propertyOffset = -1; // Slot offset in object storage
    uint8_t propertyAttrs = 0;   // Property attributes

    bool matches(uint32_t classId) const {
        return hiddenClassId == classId;
    }
};

// =============================================================================
// Inline Cache
// =============================================================================

static constexpr size_t MAX_POLY_ENTRIES = 4;

class InlineCache {
public:
    InlineCache()
        : state_(ICState::Uninitialized)
        , hitCount_(0)
        , missCount_(0)
        , polyCount_(0) {}

    // =========================================================================
    // Lookup
    // =========================================================================

    /**
     * Try to look up property offset using cached hidden class.
     * Returns >= 0 if cache hit, -1 if cache miss.
     */
    int32_t lookup(uint32_t hiddenClassId) {
        switch (state_) {
            case ICState::Uninitialized:
                missCount_++;
                return -1;

            case ICState::Monomorphic:
                if (entries_[0].matches(hiddenClassId)) {
                    hitCount_++;
                    return entries_[0].propertyOffset;
                }
                missCount_++;
                return -1;

            case ICState::Polymorphic:
                for (size_t i = 0; i < polyCount_; ++i) {
                    if (entries_[i].matches(hiddenClassId)) {
                        hitCount_++;
                        return entries_[i].propertyOffset;
                    }
                }
                missCount_++;
                return -1;

            case ICState::Megamorphic:
            case ICState::Generic:
                missCount_++;
                return -1;
        }
        return -1;
    }

    // =========================================================================
    // Update
    // =========================================================================

    /**
     * Record a successful lookup result to update the cache.
     */
    void update(uint32_t hiddenClassId, int32_t offset, uint8_t attrs = 0) {
        switch (state_) {
            case ICState::Uninitialized:
                entries_[0] = {hiddenClassId, offset, attrs};
                polyCount_ = 1;
                state_ = ICState::Monomorphic;
                break;

            case ICState::Monomorphic:
                if (entries_[0].matches(hiddenClassId)) return; // Already cached
                // Transition to polymorphic
                entries_[1] = {hiddenClassId, offset, attrs};
                polyCount_ = 2;
                state_ = ICState::Polymorphic;
                break;

            case ICState::Polymorphic:
                // Check if already present
                for (size_t i = 0; i < polyCount_; ++i) {
                    if (entries_[i].matches(hiddenClassId)) return;
                }
                if (polyCount_ < MAX_POLY_ENTRIES) {
                    entries_[polyCount_++] = {hiddenClassId, offset, attrs};
                } else {
                    // Too many entries → megamorphic
                    state_ = ICState::Megamorphic;
                }
                break;

            case ICState::Megamorphic:
                // Consider transitioning to generic if miss rate is high
                if (missCount_ > 100) {
                    state_ = ICState::Generic;
                }
                break;

            case ICState::Generic:
                break;
        }
    }

    // =========================================================================
    // Reset
    // =========================================================================

    void reset() {
        state_ = ICState::Uninitialized;
        polyCount_ = 0;
        hitCount_ = 0;
        missCount_ = 0;
    }

    // =========================================================================
    // Stats
    // =========================================================================

    ICState state() const { return state_; }
    uint32_t hitCount() const { return hitCount_; }
    uint32_t missCount() const { return missCount_; }

    double hitRate() const {
        uint32_t total = hitCount_ + missCount_;
        if (total == 0) return 0.0;
        return static_cast<double>(hitCount_) / total;
    }

    void dump(const std::string& name) const {
        printf("IC '%s': state=%s hits=%u misses=%u rate=%.1f%%\n",
               name.c_str(), icStateName(state_),
               hitCount_, missCount_, hitRate() * 100);
    }

private:
    ICState state_;
    std::array<ICEntry, MAX_POLY_ENTRIES> entries_;
    uint32_t hitCount_;
    uint32_t missCount_;
    size_t polyCount_;
};

// =============================================================================
// IC Manager (per-function IC vector)
// =============================================================================

class ICManager {
public:
    ICManager() = default;

    /**
     * Get or create IC for a given bytecode offset.
     */
    InlineCache& getIC(uint32_t bytecodeOffset) {
        if (bytecodeOffset >= caches_.size()) {
            caches_.resize(bytecodeOffset + 1);
        }
        return caches_[bytecodeOffset];
    }

    /**
     * Reset all caches (e.g., on deoptimization).
     */
    void resetAll() {
        for (auto& ic : caches_) {
            ic.reset();
        }
    }

    /**
     * Get aggregate statistics.
     */
    struct Stats {
        size_t totalICs = 0;
        size_t monomorphic = 0;
        size_t polymorphic = 0;
        size_t megamorphic = 0;
        size_t generic = 0;
        uint64_t totalHits = 0;
        uint64_t totalMisses = 0;
    };

    Stats getStats() const {
        Stats s;
        for (const auto& ic : caches_) {
            s.totalICs++;
            switch (ic.state()) {
                case ICState::Monomorphic: s.monomorphic++; break;
                case ICState::Polymorphic: s.polymorphic++; break;
                case ICState::Megamorphic: s.megamorphic++; break;
                case ICState::Generic: s.generic++; break;
                default: break;
            }
            s.totalHits += ic.hitCount();
            s.totalMisses += ic.missCount();
        }
        return s;
    }

    void dump() const {
        auto s = getStats();
        printf("IC Manager: %zu ICs (mono=%zu, poly=%zu, mega=%zu, gen=%zu)\n",
               s.totalICs, s.monomorphic, s.polymorphic, s.megamorphic, s.generic);
        printf("  Hits: %llu, Misses: %llu, Rate: %.1f%%\n",
               static_cast<unsigned long long>(s.totalHits),
               static_cast<unsigned long long>(s.totalMisses),
               s.totalHits + s.totalMisses > 0
                   ? 100.0 * s.totalHits / (s.totalHits + s.totalMisses) : 0.0);
    }

private:
    std::vector<InlineCache> caches_;
};

} // namespace Zepra::Optimization
