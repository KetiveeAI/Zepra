/**
 * @file property_storage.cpp
 * @brief Property backing store for JS objects
 *
 * Objects store properties in one of two modes:
 *
 * 1. Inline storage (fast):
 *    - Fixed number of slots embedded in the object header
 *    - Accessed by offset from object base (no indirection)
 *    - Typical: 4-8 inline slots per object
 *    - When full, overflow to out-of-line storage
 *
 * 2. Out-of-line storage (overflow):
 *    - Separate heap-allocated array of slots
 *    - Referenced by a pointer from the object header
 *    - Grows via reallocation (2x strategy)
 *    - Allows unlimited properties
 *
 * 3. Dictionary storage (degenerate):
 *    - Hash table for objects with many properties,
 *      property deletions, or non-standard patterns
 *    - More memory per property but O(1) lookup by name
 *    - No shape transitions (shape is dictionary-mode)
 *
 * GC considerations:
 * - Inline slots: traced via shape's reference map
 * - Out-of-line array: is itself a heap object (must be traced)
 * - Dictionary: hash table entries traced individually
 * - During compaction, out-of-line storage gets a forwarding pointer
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
#include <unordered_map>
#include <string>

namespace Zepra::Heap {

// =============================================================================
// Slot Value (NaN-boxed)
// =============================================================================

/**
 * @brief NaN-boxed JS value stored in object slots
 *
 * Uses 64-bit NaN-boxing:
 * - Double values stored as-is (when not NaN)
 * - Other types encoded in the NaN payload
 * - Pointer values: lower 48 bits of the 64-bit word
 * - Tag bits in the upper 16 bits distinguish types
 */
class SlotValue {
public:
    static constexpr uint64_t NAN_BITS = 0x7FF0000000000000ULL;
    static constexpr uint64_t TAG_MASK = 0xFFFF000000000000ULL;
    static constexpr uint64_t PTR_MASK = 0x0000FFFFFFFFFFFFULL;

    // Type tags (in upper 16 bits of NaN payload)
    static constexpr uint64_t TAG_OBJECT = 0x7FF1000000000000ULL;
    static constexpr uint64_t TAG_STRING = 0x7FF2000000000000ULL;
    static constexpr uint64_t TAG_INT32 = 0x7FF3000000000000ULL;
    static constexpr uint64_t TAG_BOOL = 0x7FF4000000000000ULL;
    static constexpr uint64_t TAG_NULL = 0x7FF5000000000000ULL;
    static constexpr uint64_t TAG_UNDEF = 0x7FF6000000000000ULL;
    static constexpr uint64_t TAG_SYMBOL = 0x7FF7000000000000ULL;
    static constexpr uint64_t TAG_BIGINT = 0x7FF8000000000000ULL;

    SlotValue() : bits_(TAG_UNDEF) {}
    explicit SlotValue(uint64_t raw) : bits_(raw) {}

    // Factory methods
    static SlotValue fromDouble(double d) {
        SlotValue v;
        std::memcpy(&v.bits_, &d, sizeof(double));
        return v;
    }

    static SlotValue fromObject(void* obj) {
        return SlotValue(TAG_OBJECT | (reinterpret_cast<uintptr_t>(obj) & PTR_MASK));
    }

    static SlotValue fromString(void* str) {
        return SlotValue(TAG_STRING | (reinterpret_cast<uintptr_t>(str) & PTR_MASK));
    }

    static SlotValue fromInt32(int32_t i) {
        return SlotValue(TAG_INT32 | static_cast<uint32_t>(i));
    }

    static SlotValue fromBool(bool b) {
        return SlotValue(TAG_BOOL | (b ? 1ULL : 0ULL));
    }

    static SlotValue null() { return SlotValue(TAG_NULL); }
    static SlotValue undefined() { return SlotValue(TAG_UNDEF); }

    // Type checks
    bool isDouble() const {
        return (bits_ & NAN_BITS) != NAN_BITS ||
               bits_ == 0x7FF0000000000000ULL; // +Infinity
    }
    bool isObject() const { return (bits_ & TAG_MASK) == TAG_OBJECT; }
    bool isString() const { return (bits_ & TAG_MASK) == TAG_STRING; }
    bool isInt32() const { return (bits_ & TAG_MASK) == TAG_INT32; }
    bool isBool() const { return (bits_ & TAG_MASK) == TAG_BOOL; }
    bool isNull() const { return bits_ == TAG_NULL; }
    bool isUndefined() const { return bits_ == TAG_UNDEF; }
    bool isSymbol() const { return (bits_ & TAG_MASK) == TAG_SYMBOL; }
    bool isBigInt() const { return (bits_ & TAG_MASK) == TAG_BIGINT; }

    /**
     * @brief Check if this value contains a heap pointer (for GC)
     */
    bool isHeapPointer() const {
        return isObject() || isString() || isSymbol() || isBigInt();
    }

    // Value extraction
    double asDouble() const {
        double d;
        std::memcpy(&d, &bits_, sizeof(double));
        return d;
    }

    void* asPointer() const {
        return reinterpret_cast<void*>(bits_ & PTR_MASK);
    }

    int32_t asInt32() const {
        return static_cast<int32_t>(bits_ & 0xFFFFFFFF);
    }

    bool asBool() const { return (bits_ & 1) != 0; }

    uint64_t raw() const { return bits_; }

    /**
     * @brief Update the pointer (for GC compaction reference fixup)
     */
    void updatePointer(void* newPtr) {
        uint64_t tag = bits_ & TAG_MASK;
        bits_ = tag | (reinterpret_cast<uintptr_t>(newPtr) & PTR_MASK);
    }

    bool operator==(const SlotValue& other) const {
        return bits_ == other.bits_;
    }

private:
    uint64_t bits_;
};

static_assert(sizeof(SlotValue) == 8, "SlotValue must be 8 bytes");

// =============================================================================
// Inline Property Storage
// =============================================================================

static constexpr size_t MAX_INLINE_SLOTS = 8;

/**
 * @brief Fixed inline slots embedded in object header
 *
 * These slots are part of the object's allocation.
 * The object header is:
 *   [shape pointer (8 bytes)] [hash (4 bytes)] [flags (4 bytes)]
 *   [inline slot 0 (8 bytes)] ... [inline slot N-1 (8 bytes)]
 */
struct InlineStorage {
    SlotValue slots[MAX_INLINE_SLOTS];

    void clear() {
        for (size_t i = 0; i < MAX_INLINE_SLOTS; i++) {
            slots[i] = SlotValue::undefined();
        }
    }

    SlotValue get(uint16_t index) const {
        assert(index < MAX_INLINE_SLOTS);
        return slots[index];
    }

    void set(uint16_t index, SlotValue value) {
        assert(index < MAX_INLINE_SLOTS);
        slots[index] = value;
    }

    /**
     * @brief Trace all reference slots (for GC)
     */
    void traceReferences(const std::vector<bool>& refMap,
                          std::function<void(void**)> visitor) {
        for (size_t i = 0; i < MAX_INLINE_SLOTS && i < refMap.size(); i++) {
            if (refMap[i] && slots[i].isHeapPointer()) {
                void* ptr = slots[i].asPointer();
                visitor(&ptr);
                if (ptr != slots[i].asPointer()) {
                    slots[i].updatePointer(ptr);
                }
            }
        }
    }
};

// =============================================================================
// Out-of-line Property Storage
// =============================================================================

/**
 * @brief Dynamically-sized overflow storage
 *
 * Allocated as a separate heap object when inline slots overflow.
 * Stored in a contiguous array of SlotValues.
 */
class OutOfLineStorage {
public:
    OutOfLineStorage() : slots_(nullptr), capacity_(0) {}

    ~OutOfLineStorage() {
        delete[] slots_;
    }

    OutOfLineStorage(const OutOfLineStorage&) = delete;
    OutOfLineStorage& operator=(const OutOfLineStorage&) = delete;

    /**
     * @brief Ensure at least 'needed' slots are available
     */
    bool ensureCapacity(size_t needed) {
        if (needed <= capacity_) return true;

        size_t newCap = capacity_ == 0 ? 4 : capacity_ * 2;
        while (newCap < needed) newCap *= 2;

        auto* newSlots = new (std::nothrow) SlotValue[newCap];
        if (!newSlots) return false;

        // Initialize to undefined
        for (size_t i = 0; i < newCap; i++) {
            newSlots[i] = SlotValue::undefined();
        }

        // Copy existing
        if (slots_) {
            for (size_t i = 0; i < capacity_; i++) {
                newSlots[i] = slots_[i];
            }
            delete[] slots_;
        }

        slots_ = newSlots;
        capacity_ = newCap;
        return true;
    }

    SlotValue get(size_t index) const {
        if (index >= capacity_) return SlotValue::undefined();
        return slots_[index];
    }

    void set(size_t index, SlotValue value) {
        assert(index < capacity_);
        slots_[index] = value;
    }

    size_t capacity() const { return capacity_; }

    /**
     * @brief Trace all reference slots (for GC)
     */
    void traceReferences(const std::vector<bool>& refMap,
                          size_t inlineCount,
                          std::function<void(void**)> visitor) {
        for (size_t i = 0; i < capacity_; i++) {
            size_t globalIndex = inlineCount + i;
            if (globalIndex < refMap.size() && refMap[globalIndex]) {
                if (slots_[i].isHeapPointer()) {
                    void* ptr = slots_[i].asPointer();
                    visitor(&ptr);
                    if (ptr != slots_[i].asPointer()) {
                        slots_[i].updatePointer(ptr);
                    }
                }
            }
        }
    }

    /**
     * @brief Get raw pointer for GC to trace this object itself
     */
    SlotValue** storageRoot() { return &slots_; }

private:
    SlotValue* slots_;
    size_t capacity_;
};

// =============================================================================
// Dictionary Property Storage
// =============================================================================

/**
 * @brief Hash-table based property storage
 *
 * Used when shapes degenerate (too many transitions, deletions).
 * Each entry stores name, value, and attributes.
 */
class DictionaryStorage {
public:
    struct Entry {
        uint32_t nameId;
        SlotValue value;
        uint8_t attrs;
        bool isReference;
        bool occupied;

        Entry() : nameId(0), attrs(0), isReference(false), occupied(false) {}
    };

    DictionaryStorage() : size_(0) {
        entries_.resize(INITIAL_CAPACITY);
    }

    SlotValue get(uint32_t nameId) const {
        size_t idx = findEntry(nameId);
        if (idx == SIZE_MAX) return SlotValue::undefined();
        return entries_[idx].value;
    }

    bool set(uint32_t nameId, SlotValue value, uint8_t attrs,
             bool isRef) {
        size_t idx = findEntry(nameId);
        if (idx != SIZE_MAX) {
            entries_[idx].value = value;
            return true;
        }

        // Check load factor
        if (static_cast<double>(size_ + 1) >
            static_cast<double>(entries_.size()) * 0.75) {
            grow();
        }

        idx = insertProbe(nameId);
        entries_[idx].nameId = nameId;
        entries_[idx].value = value;
        entries_[idx].attrs = attrs;
        entries_[idx].isReference = isRef;
        entries_[idx].occupied = true;
        size_++;
        return true;
    }

    bool remove(uint32_t nameId) {
        size_t idx = findEntry(nameId);
        if (idx == SIZE_MAX) return false;
        entries_[idx].occupied = false;
        entries_[idx].value = SlotValue::undefined();
        size_--;
        return true;
    }

    bool has(uint32_t nameId) const {
        return findEntry(nameId) != SIZE_MAX;
    }

    size_t size() const { return size_; }

    /**
     * @brief Trace all reference values (for GC)
     */
    void traceReferences(std::function<void(void**)> visitor) {
        for (auto& entry : entries_) {
            if (entry.occupied && entry.isReference &&
                entry.value.isHeapPointer()) {
                void* ptr = entry.value.asPointer();
                visitor(&ptr);
                if (ptr != entry.value.asPointer()) {
                    entry.value.updatePointer(ptr);
                }
            }
        }
    }

    void forEach(std::function<void(uint32_t nameId,
                                     const SlotValue& value)> visitor) const {
        for (const auto& entry : entries_) {
            if (entry.occupied) {
                visitor(entry.nameId, entry.value);
            }
        }
    }

private:
    static constexpr size_t INITIAL_CAPACITY = 16;

    size_t hash(uint32_t nameId) const {
        return static_cast<size_t>(nameId * 2654435761u) % entries_.size();
    }

    size_t findEntry(uint32_t nameId) const {
        size_t idx = hash(nameId);
        for (size_t i = 0; i < entries_.size(); i++) {
            size_t pos = (idx + i) % entries_.size();
            if (!entries_[pos].occupied) {
                if (entries_[pos].nameId == 0) return SIZE_MAX;
                continue;
            }
            if (entries_[pos].nameId == nameId) return pos;
        }
        return SIZE_MAX;
    }

    size_t insertProbe(uint32_t nameId) {
        size_t idx = hash(nameId);
        for (size_t i = 0; i < entries_.size(); i++) {
            size_t pos = (idx + i) % entries_.size();
            if (!entries_[pos].occupied) return pos;
        }
        return 0; // Should never hit (we grow before full)
    }

    void grow() {
        auto old = std::move(entries_);
        entries_.resize(old.size() * 2);
        size_ = 0;
        for (auto& e : old) {
            if (e.occupied) {
                set(e.nameId, e.value, e.attrs, e.isReference);
            }
        }
    }

    std::vector<Entry> entries_;
    size_t size_;
};

} // namespace Zepra::Heap
