/**
 * @file ObjectLayout.h
 * @brief Object memory layout and structure
 * 
 * Implements:
 * - Object header layout
 * - Slot access patterns
 * - Object flags and metadata
 * - GC integration
 * 
 * Based on V8 JSObject layout
 */

#pragma once

#include "Shape.h"
#include "PropertyStorage.h"
#include <cstdint>
#include <atomic>

namespace Zepra::Runtime {

// =============================================================================
// Object Flags
// =============================================================================

enum class ObjectFlags : uint32_t {
    None            = 0,
    Frozen          = 1 << 0,   // Object.freeze
    Sealed          = 1 << 1,   // Object.seal
    NonExtensible   = 1 << 2,   // Object.preventExtensions
    Prototype       = 1 << 3,   // Used as prototype
    Callable        = 1 << 4,   // Has [[Call]]
    Constructor     = 1 << 5,   // Has [[Construct]]
    Array           = 1 << 6,   // Is array exotic
    Proxy           = 1 << 7,   // Is proxy
    NeedsBarrier    = 1 << 8,   // Write barrier needed
    IsNativeObject  = 1 << 9,   // Has native slots
    HasIndexedProperties = 1 << 10
};

inline ObjectFlags operator|(ObjectFlags a, ObjectFlags b) {
    return static_cast<ObjectFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ObjectFlags operator&(ObjectFlags a, ObjectFlags b) {
    return static_cast<ObjectFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasFlag(ObjectFlags flags, ObjectFlags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// =============================================================================
// Object Header
// =============================================================================

/**
 * @brief Object header containing metadata
 * 
 * Layout in memory:
 * +----------------+
 * | Shape pointer  | 8 bytes
 * +----------------+
 * | GC header      | 8 bytes (forwarding ptr, mark bits)
 * +----------------+
 * | Flags          | 4 bytes
 * +----------------+
 * | Reserved       | 4 bytes
 * +----------------+
 * | Elements ptr   | 8 bytes (for arrays)
 * +----------------+
 */
struct ObjectHeader {
    Shape* shape;
    
    // GC metadata
    struct GCMeta {
        std::atomic<uintptr_t> bits;
        
        static constexpr uintptr_t MARK_BIT = 1ULL << 0;
        static constexpr uintptr_t PINNED_BIT = 1ULL << 1;
        static constexpr uintptr_t FORWARDED_BIT = 1ULL << 2;
        static constexpr uintptr_t FORWARDING_MASK = ~0x7ULL;
        
        bool isMarked() const { return bits.load() & MARK_BIT; }
        void setMarked() { bits.fetch_or(MARK_BIT); }
        void clearMarked() { bits.fetch_and(~MARK_BIT); }
        
        bool isForwarded() const { return bits.load() & FORWARDED_BIT; }
        void* forwardingAddress() const {
            return reinterpret_cast<void*>(bits.load() & FORWARDING_MASK);
        }
        void setForwarding(void* addr) {
            bits.store(reinterpret_cast<uintptr_t>(addr) | FORWARDED_BIT);
        }
    } gc;
    
    ObjectFlags flags;
    uint32_t reserved;
    
    ElementStorage* elements;
    
    // Helpers
    bool isFrozen() const { return hasFlag(flags, ObjectFlags::Frozen); }
    bool isSealed() const { return hasFlag(flags, ObjectFlags::Sealed); }
    bool isExtensible() const { return !hasFlag(flags, ObjectFlags::NonExtensible); }
    bool isCallable() const { return hasFlag(flags, ObjectFlags::Callable); }
    bool isConstructor() const { return hasFlag(flags, ObjectFlags::Constructor); }
    bool isArray() const { return hasFlag(flags, ObjectFlags::Array); }
    bool isProxy() const { return hasFlag(flags, ObjectFlags::Proxy); }
};

// =============================================================================
// Object Layout
// =============================================================================

/**
 * @brief Describes the memory layout of an object
 */
class ObjectLayout {
public:
    static constexpr size_t HEADER_SIZE = sizeof(ObjectHeader);
    static constexpr size_t PROPERTY_SLOT_SIZE = sizeof(Value);
    
    /**
     * @brief Calculate total object size
     */
    static size_t size(size_t propertyCount, size_t internalSlots = 0) {
        return HEADER_SIZE + 
               PropertyStorage::INLINE_CAPACITY * PROPERTY_SLOT_SIZE +
               internalSlots * PROPERTY_SLOT_SIZE;
    }
    
    /**
     * @brief Get header from object pointer
     */
    static ObjectHeader* getHeader(void* obj) {
        return static_cast<ObjectHeader*>(obj);
    }
    
    static const ObjectHeader* getHeader(const void* obj) {
        return static_cast<const ObjectHeader*>(obj);
    }
    
    /**
     * @brief Get property storage start
     */
    static Value* getPropertyStart(void* obj) {
        return reinterpret_cast<Value*>(
            static_cast<char*>(obj) + HEADER_SIZE);
    }
    
    /**
     * @brief Get property by offset
     */
    static Value getProperty(void* obj, uint32_t offset) {
        return getPropertyStart(obj)[offset];
    }
    
    static void setProperty(void* obj, uint32_t offset, Value value) {
        getPropertyStart(obj)[offset] = value;
    }
    
    /**
     * @brief Get internal slot (for native objects)
     */
    template<typename T>
    static T* getInternalSlot(void* obj, size_t slotIndex) {
        size_t offset = HEADER_SIZE + 
                        PropertyStorage::INLINE_CAPACITY * PROPERTY_SLOT_SIZE +
                        slotIndex * sizeof(T);
        return reinterpret_cast<T*>(static_cast<char*>(obj) + offset);
    }
};

// =============================================================================
// Slot Accessor
// =============================================================================

/**
 * @brief Type-safe slot accessor for internal fields
 */
template<typename T, size_t SlotIndex>
class InternalSlot {
public:
    static T get(void* obj) {
        return *ObjectLayout::getInternalSlot<T>(obj, SlotIndex);
    }
    
    static void set(void* obj, T value) {
        *ObjectLayout::getInternalSlot<T>(obj, SlotIndex) = value;
    }
    
    static T* ptr(void* obj) {
        return ObjectLayout::getInternalSlot<T>(obj, SlotIndex);
    }
};

// =============================================================================
// Object Prototype Chain
// =============================================================================

class PrototypeChain {
public:
    /**
     * @brief Get prototype of object
     */
    static void* getPrototype(void* obj);
    
    /**
     * @brief Set prototype of object
     */
    static bool setPrototype(void* obj, void* proto);
    
    /**
     * @brief Check prototype chain for property
     */
    static void* lookupPropertyHolder(void* obj, const std::string& name);
    
    /**
     * @brief Detect cycle in prototype chain
     */
    static bool wouldCreateCycle(void* obj, void* proto);
    
    /**
     * @brief Get chain length
     */
    static size_t chainLength(void* obj);
    
    static constexpr size_t MAX_CHAIN_LENGTH = 1000;
};

} // namespace Zepra::Runtime
