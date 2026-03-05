/**
 * @file object_layout.cpp
 * @brief GC-integrated object header and layout management
 *
 * Every heap object has a common header:
 *   ┌─────────────────────────────────────────────────┐
 *   │  Word 0: Mark word (GC bits + hash + forwarding)│
 *   │  Word 1: Shape pointer (hidden class)           │
 *   │  Word 2+: Inline property slots                 │
 *   └─────────────────────────────────────────────────┘
 *
 * Mark word encoding (64 bits):
 *   [63]     Forwarded flag (1 = object has been moved)
 *   [62]     Marked flag (1 = reachable)
 *   [61]     Pinned flag (1 = cannot be moved)
 *   [60]     Finalized flag (1 = destructor has run)
 *   [59:56]  Age (4 bits, 0-15 for generational aging)
 *   [55:32]  Identity hash (24 bits)
 *   [31:0]   Forwarding address lower bits / lock bits
 *
 * Shape pointer:
 *   Points to the Shape (hidden class) that describes
 *   this object's property layout. The shape tells the
 *   GC exactly which slots are references.
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>

namespace Zepra::Heap {

// =============================================================================
// Mark Word
// =============================================================================

class MarkWord {
public:
    static constexpr uint64_t FORWARDED_BIT = uint64_t(1) << 63;
    static constexpr uint64_t MARKED_BIT = uint64_t(1) << 62;
    static constexpr uint64_t PINNED_BIT = uint64_t(1) << 61;
    static constexpr uint64_t FINALIZED_BIT = uint64_t(1) << 60;
    static constexpr uint64_t AGE_SHIFT = 56;
    static constexpr uint64_t AGE_MASK = uint64_t(0xF) << AGE_SHIFT;
    static constexpr uint64_t HASH_SHIFT = 32;
    static constexpr uint64_t HASH_MASK = uint64_t(0xFFFFFF) << HASH_SHIFT;
    static constexpr uint64_t FORWARD_MASK = 0xFFFFFFFFULL;

    MarkWord() : bits_(0) {}
    explicit MarkWord(uint64_t raw) : bits_(raw) {}

    // -------------------------------------------------------------------------
    // GC mark bit
    // -------------------------------------------------------------------------

    bool isMarked() const { return (bits_ & MARKED_BIT) != 0; }

    void setMarked() { bits_ |= MARKED_BIT; }
    void clearMarked() { bits_ &= ~MARKED_BIT; }

    /**
     * @brief Atomic CAS mark (for concurrent marking)
     */
    bool tryMark(std::atomic<uint64_t>& atomicBits) {
        uint64_t expected = atomicBits.load(std::memory_order_relaxed);
        while (!(expected & MARKED_BIT)) {
            uint64_t desired = expected | MARKED_BIT;
            if (atomicBits.compare_exchange_weak(expected, desired,
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return true;  // We marked it
            }
        }
        return false;  // Already marked
    }

    // -------------------------------------------------------------------------
    // Forwarding (for compaction)
    // -------------------------------------------------------------------------

    bool isForwarded() const { return (bits_ & FORWARDED_BIT) != 0; }

    uintptr_t forwardingAddress() const {
        // Full pointer stored externally; lower 32 bits here as hint
        return static_cast<uintptr_t>(bits_ & FORWARD_MASK);
    }

    void setForwarded(uint32_t addrLow) {
        bits_ = FORWARDED_BIT | static_cast<uint64_t>(addrLow);
    }

    // -------------------------------------------------------------------------
    // Age (for generational GC)
    // -------------------------------------------------------------------------

    uint8_t age() const {
        return static_cast<uint8_t>((bits_ & AGE_MASK) >> AGE_SHIFT);
    }

    void setAge(uint8_t a) {
        bits_ = (bits_ & ~AGE_MASK) | (static_cast<uint64_t>(a & 0xF) << AGE_SHIFT);
    }

    void incrementAge() {
        uint8_t a = age();
        if (a < 15) setAge(a + 1);
    }

    // -------------------------------------------------------------------------
    // Pinning
    // -------------------------------------------------------------------------

    bool isPinned() const { return (bits_ & PINNED_BIT) != 0; }
    void setPinned() { bits_ |= PINNED_BIT; }
    void clearPinned() { bits_ &= ~PINNED_BIT; }

    // -------------------------------------------------------------------------
    // Finalization
    // -------------------------------------------------------------------------

    bool isFinalized() const { return (bits_ & FINALIZED_BIT) != 0; }
    void setFinalized() { bits_ |= FINALIZED_BIT; }

    // -------------------------------------------------------------------------
    // Identity hash
    // -------------------------------------------------------------------------

    uint32_t identityHash() const {
        return static_cast<uint32_t>((bits_ & HASH_MASK) >> HASH_SHIFT);
    }

    void setIdentityHash(uint32_t hash) {
        bits_ = (bits_ & ~HASH_MASK) |
                (static_cast<uint64_t>(hash & 0xFFFFFF) << HASH_SHIFT);
    }

    uint64_t raw() const { return bits_; }

private:
    uint64_t bits_;
};

// =============================================================================
// Object Header
// =============================================================================

static constexpr size_t OBJECT_HEADER_SIZE = 16;  // 2 words

/**
 * @brief Common header for all heap objects
 *
 * Word 0: Mark word (atomic for concurrent GC)
 * Word 1: Shape pointer
 */
struct ObjectHeader {
    std::atomic<uint64_t> markWord;
    void* shapePtr;

    MarkWord mark() const {
        return MarkWord(markWord.load(std::memory_order_acquire));
    }

    void setMark(MarkWord m) {
        markWord.store(m.raw(), std::memory_order_release);
    }

    bool tryMark() {
        MarkWord m;
        return m.tryMark(markWord);
    }

    template<typename ShapeType>
    ShapeType* shape() const {
        return static_cast<ShapeType*>(shapePtr);
    }

    void setShape(void* shape) {
        shapePtr = shape;
    }
};

static_assert(sizeof(ObjectHeader) == OBJECT_HEADER_SIZE,
              "ObjectHeader must be 16 bytes");

// =============================================================================
// Object Layout Manager
// =============================================================================

/**
 * @brief Manages object sizes and slot calculations
 *
 * Given a shape, computes:
 * - Total object size (header + inline slots)
 * - Whether out-of-line storage is needed
 * - Slot offsets for property access
 */
class ObjectLayoutManager {
public:
    static constexpr size_t MAX_INLINE_SLOTS = 8;

    /**
     * @brief Compute total allocation size for an object
     */
    static size_t computeAllocationSize(uint16_t slotCount) {
        size_t inlineSlots = std::min(static_cast<size_t>(slotCount),
                                       MAX_INLINE_SLOTS);
        size_t size = OBJECT_HEADER_SIZE + inlineSlots * sizeof(uint64_t);
        return (size + 7) & ~size_t(7);  // Align to 8
    }

    /**
     * @brief Check if object needs out-of-line storage
     */
    static bool needsOutOfLine(uint16_t slotCount) {
        return slotCount > MAX_INLINE_SLOTS;
    }

    /**
     * @brief Get slot address relative to object base
     */
    static size_t inlineSlotOffset(uint16_t slotIndex) {
        return OBJECT_HEADER_SIZE + slotIndex * sizeof(uint64_t);
    }

    /**
     * @brief Get out-of-line slot index (0-based within OOL storage)
     */
    static uint16_t outOfLineIndex(uint16_t slotIndex) {
        return slotIndex - static_cast<uint16_t>(MAX_INLINE_SLOTS);
    }

    /**
     * @brief Read a slot value from object memory
     */
    static uint64_t readSlot(uintptr_t objBase, uint16_t slotIndex) {
        if (slotIndex < MAX_INLINE_SLOTS) {
            uintptr_t addr = objBase + inlineSlotOffset(slotIndex);
            return *reinterpret_cast<uint64_t*>(addr);
        }
        // Out-of-line: read OOL pointer from a reserved inline slot,
        // then index into the OOL array
        return 0;  // Requires OOL pointer chase
    }

    /**
     * @brief Write a slot value to object memory
     */
    static void writeSlot(uintptr_t objBase, uint16_t slotIndex,
                           uint64_t value) {
        if (slotIndex < MAX_INLINE_SLOTS) {
            uintptr_t addr = objBase + inlineSlotOffset(slotIndex);
            *reinterpret_cast<uint64_t*>(addr) = value;
        }
    }
};

// =============================================================================
// Object Tracer (GC integration)
// =============================================================================

/**
 * @brief Traces heap references within an object
 *
 * Uses the object's shape to determine which slots contain
 * heap pointers. This is the core of GC marking.
 */
class ObjectTracer {
public:
    using VisitorFn = std::function<void(void** slot)>;

    /**
     * @brief Trace all heap references in an object
     *
     * @param objBase Base address of the object
     * @param refMap Reference map from the object's shape
     * @param slotCount Total number of property slots
     * @param visitor Called for each reference slot
     */
    static void trace(uintptr_t objBase,
                       const std::vector<bool>& refMap,
                       uint16_t slotCount,
                       VisitorFn visitor) {
        // Trace shape pointer
        auto* header = reinterpret_cast<ObjectHeader*>(objBase);
        if (header->shapePtr) {
            visitor(&header->shapePtr);
        }

        // Trace inline slots
        size_t inlineCount = std::min(
            static_cast<size_t>(slotCount),
            ObjectLayoutManager::MAX_INLINE_SLOTS);

        for (size_t i = 0; i < inlineCount; i++) {
            if (i < refMap.size() && refMap[i]) {
                uintptr_t slotAddr = objBase +
                    ObjectLayoutManager::inlineSlotOffset(
                        static_cast<uint16_t>(i));
                auto* slotPtr = reinterpret_cast<void**>(slotAddr);
                if (*slotPtr) {
                    visitor(slotPtr);
                }
            }
        }
    }

    /**
     * @brief Compute the GC cost of tracing an object
     *
     * Used by the scheduler to estimate marking work.
     * More references = more work per object.
     */
    static size_t traceCost(const std::vector<bool>& refMap) {
        size_t cost = 1;  // Base cost (header trace)
        for (bool b : refMap) if (b) cost++;
        return cost;
    }
};

// =============================================================================
// Object Size Calculator
// =============================================================================

/**
 * @brief Computes object sizes for GC sweep
 *
 * During sweep, the GC needs to know each object's size
 * to walk the heap (object-by-object). The size comes from
 * the object's shape.
 */
class ObjectSizeCalculator {
public:
    using SizeResolver = std::function<size_t(void* shapePtr)>;

    explicit ObjectSizeCalculator(SizeResolver resolver)
        : resolver_(std::move(resolver)) {}

    /**
     * @brief Get the size of an object at the given address
     */
    size_t objectSize(uintptr_t objAddr) const {
        auto* header = reinterpret_cast<ObjectHeader*>(objAddr);
        if (!header->shapePtr) {
            return OBJECT_HEADER_SIZE;  // Tombstone or corrupted
        }

        if (resolver_) {
            return resolver_(header->shapePtr);
        }

        return OBJECT_HEADER_SIZE;
    }

    /**
     * @brief Walk objects in a memory range
     */
    void walkObjects(uintptr_t start, uintptr_t end,
                      std::function<void(uintptr_t objAddr,
                                          size_t objSize)> visitor) const {
        uintptr_t cursor = start;
        while (cursor < end) {
            size_t size = objectSize(cursor);
            if (size == 0 || size > (end - cursor)) break;

            visitor(cursor, size);
            cursor += size;
        }
    }

private:
    SizeResolver resolver_;
};

// =============================================================================
// Object Initializer
// =============================================================================

/**
 * @brief Initializes freshly-allocated objects
 *
 * Sets up the header, shape, and initializes slots to undefined.
 */
class ObjectInitializer {
public:
    /**
     * @brief Initialize a new object
     *
     * @param addr Allocated memory address
     * @param shape Shape for this object
     * @param slotCount Number of property slots
     * @return Pointer to the initialized header
     */
    static ObjectHeader* initialize(uintptr_t addr, void* shape,
                                     uint16_t slotCount) {
        auto* header = reinterpret_cast<ObjectHeader*>(addr);

        // Set mark word (clean: no marks, age 0, no hash)
        header->markWord.store(0, std::memory_order_release);

        // Set shape
        header->shapePtr = shape;

        // Zero all inline slots
        size_t inlineCount = std::min(
            static_cast<size_t>(slotCount),
            ObjectLayoutManager::MAX_INLINE_SLOTS);

        if (inlineCount > 0) {
            uintptr_t slotsStart = addr + OBJECT_HEADER_SIZE;
            std::memset(reinterpret_cast<void*>(slotsStart), 0,
                        inlineCount * sizeof(uint64_t));
        }

        return header;
    }

    /**
     * @brief Generate identity hash for an object
     *
     * Lazily generated on first hashCode() call.
     * Uses the object's address mixed with a counter.
     */
    static uint32_t generateHash(uintptr_t objAddr) {
        static std::atomic<uint32_t> counter{0};
        uint32_t c = counter.fetch_add(1, std::memory_order_relaxed);
        uint32_t h = static_cast<uint32_t>(objAddr >> 3) ^ (c * 2654435761u);
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        return h & 0xFFFFFF;  // 24-bit hash
    }
};

} // namespace Zepra::Heap
