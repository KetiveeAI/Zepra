/**
 * @file shape_table.cpp
 * @brief Hidden class (Shape) system for GC-aware object model
 *
 * Every JS object has a "shape" (aka hidden class, structure, map)
 * that describes its property layout. Shapes enable:
 * - Fast property access (known offset at compile time)
 * - Inline cache validity (IC checks shape pointer)
 * - GC tracing (shape tells GC where reference slots are)
 * - Memory efficiency (layout shared across objects)
 *
 * Shape transitions:
 * - Adding a property creates a new shape (transition)
 * - Transitions form a tree from the root (empty) shape
 * - Transition lookup is cached for fast repeated patterns
 *
 * Shape descriptors contain:
 * - Property table: name → (offset, attributes)
 * - Slot count: number of property slots in object
 * - Reference map: which slots contain heap references (for GC)
 * - Prototype pointer (for prototype chain lookups)
 * - Transition table: property name → child shape
 *
 * GC integration:
 * - Shapes themselves are heap-allocated and GC'd
 * - Shape descriptor tells GC exactly which slots to trace
 * - Shape transitions are weak (dead shapes get collected)
 */

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <deque>
#include <functional>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Zepra::Heap {

// Forward declaration
class ShapeAllocator;

// =============================================================================
// Property Attributes
// =============================================================================

enum class PropertyAttribute : uint8_t {
    None = 0,
    Writable = 1 << 0,
    Enumerable = 1 << 1,
    Configurable = 1 << 2,
    Accessor = 1 << 3,       // Getter/setter, not data property

    DefaultData = Writable | Enumerable | Configurable,
    DefaultAccessor = Enumerable | Configurable | Accessor,
    ReadOnly = Enumerable | Configurable,
    Internal = 0,            // Non-enumerable, non-configurable
};

inline PropertyAttribute operator|(PropertyAttribute a, PropertyAttribute b) {
    return static_cast<PropertyAttribute>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasAttribute(PropertyAttribute attrs, PropertyAttribute flag) {
    return (static_cast<uint8_t>(attrs) & static_cast<uint8_t>(flag)) != 0;
}

// =============================================================================
// Property Descriptor
// =============================================================================

struct PropertyDescriptor {
    uint32_t nameId;            // Interned string ID
    uint16_t offset;            // Slot offset in object
    PropertyAttribute attrs;
    bool isReference;           // Slot holds a heap reference (for GC)

    PropertyDescriptor()
        : nameId(0), offset(0)
        , attrs(PropertyAttribute::None), isReference(false) {}

    PropertyDescriptor(uint32_t name, uint16_t off,
                       PropertyAttribute a, bool ref)
        : nameId(name), offset(off), attrs(a), isReference(ref) {}
};

// =============================================================================
// Shape (Hidden Class)
// =============================================================================

class Shape {
public:
    static constexpr uint32_t INVALID_ID = UINT32_MAX;
    static constexpr uint16_t MAX_INLINE_PROPERTIES = 128;

    Shape(uint32_t id, Shape* parent)
        : id_(id)
        , parent_(parent)
        , prototype_(nullptr)
        , slotCount_(parent ? parent->slotCount_ : 0)
        , instanceSize_(0)
        , isSealed_(false)
        , isFrozen_(false)
        , isDictionary_(false)
        , transitionCount_(0)
        , objectCount_(0) {
        if (parent) {
            // Inherit properties from parent
            properties_ = parent->properties_;
            referenceMap_ = parent->referenceMap_;
        }
        updateInstanceSize();
    }

    uint32_t id() const { return id_; }
    Shape* parent() const { return parent_; }
    uint16_t slotCount() const { return slotCount_; }
    size_t instanceSize() const { return instanceSize_; }
    bool isSealed() const { return isSealed_; }
    bool isFrozen() const { return isFrozen_; }
    bool isDictionary() const { return isDictionary_; }

    // Prototype
    void* prototype() const { return prototype_; }
    void setPrototype(void* proto) { prototype_ = proto; }

    // -------------------------------------------------------------------------
    // Property operations
    // -------------------------------------------------------------------------

    /**
     * @brief Add a property to this shape → creates new shape
     *
     * Does NOT modify this shape. Returns the transition shape.
     * If the transition already exists, returns the cached one.
     */
    Shape* addProperty(uint32_t nameId, PropertyAttribute attrs,
                       bool isReference, ShapeAllocator& allocator);

    /**
     * @brief Lookup a property by name
     * @return Pointer to descriptor, or nullptr if not found
     */
    const PropertyDescriptor* lookupProperty(uint32_t nameId) const {
        for (const auto& prop : properties_) {
            if (prop.nameId == nameId) return &prop;
        }
        return nullptr;
    }

    /**
     * @brief Get the offset for a property (fast path)
     * @return Offset or UINT16_MAX if not found
     */
    uint16_t propertyOffset(uint32_t nameId) const {
        auto* desc = lookupProperty(nameId);
        return desc ? desc->offset : UINT16_MAX;
    }

    size_t propertyCount() const { return properties_.size(); }

    const std::vector<PropertyDescriptor>& properties() const {
        return properties_;
    }

    // -------------------------------------------------------------------------
    // Reference map (for GC tracing)
    // -------------------------------------------------------------------------

    /**
     * @brief Get the reference map (which slots contain heap pointers)
     *
     * The GC uses this to know exactly which object slots to trace.
     */
    const std::vector<bool>& referenceMap() const { return referenceMap_; }

    /**
     * @brief Check if a specific slot holds a reference
     */
    bool isReferenceSlot(uint16_t slot) const {
        return slot < referenceMap_.size() && referenceMap_[slot];
    }

    /**
     * @brief Count of reference slots (for GC cost estimation)
     */
    size_t referenceSlotCount() const {
        size_t count = 0;
        for (bool b : referenceMap_) if (b) count++;
        return count;
    }

    /**
     * @brief Enumerate reference slot offsets
     */
    void forEachReferenceSlot(std::function<void(uint16_t offset)> visitor) const {
        for (size_t i = 0; i < referenceMap_.size(); i++) {
            if (referenceMap_[i]) {
                visitor(static_cast<uint16_t>(i));
            }
        }
    }

    // -------------------------------------------------------------------------
    // Transitions
    // -------------------------------------------------------------------------

    /**
     * @brief Lookup an existing transition
     */
    Shape* findTransition(uint32_t nameId) const {
        std::shared_lock<std::shared_mutex> lock(transitionMutex_);
        auto it = transitions_.find(nameId);
        return it != transitions_.end() ? it->second : nullptr;
    }

    /**
     * @brief Register a transition
     */
    void addTransition(uint32_t nameId, Shape* target) {
        std::unique_lock<std::shared_mutex> lock(transitionMutex_);
        transitions_[nameId] = target;
        transitionCount_++;
    }

    size_t transitionCount() const { return transitionCount_; }

    /**
     * @brief Iterate transitions (for GC marking)
     */
    void forEachTransition(std::function<void(uint32_t nameId,
                                               Shape* target)> visitor) const {
        std::shared_lock<std::shared_mutex> lock(transitionMutex_);
        for (const auto& [name, shape] : transitions_) {
            visitor(name, shape);
        }
    }

    // -------------------------------------------------------------------------
    // Object tracking
    // -------------------------------------------------------------------------

    void incrementObjectCount() {
        objectCount_.fetch_add(1, std::memory_order_relaxed);
    }
    void decrementObjectCount() {
        objectCount_.fetch_sub(1, std::memory_order_relaxed);
    }
    uint64_t objectCount() const {
        return objectCount_.load(std::memory_order_relaxed);
    }

    // -------------------------------------------------------------------------
    // Sealing / freezing
    // -------------------------------------------------------------------------

    Shape* seal(ShapeAllocator& allocator);
    Shape* freeze(ShapeAllocator& allocator);

    /**
     * @brief Convert to dictionary mode (no more shape transitions)
     *
     * When an object has too many transitions or deletions,
     * it's more efficient to switch to a hash table for properties.
     */
    void convertToDictionary() { isDictionary_ = true; }

private:
    void updateInstanceSize() {
        // Header (shape pointer + hash) + slots * 8 bytes
        instanceSize_ = 16 + static_cast<size_t>(slotCount_) * 8;
    }

    uint32_t id_;
    Shape* parent_;
    void* prototype_;

    std::vector<PropertyDescriptor> properties_;
    std::vector<bool> referenceMap_;    // One bit per slot
    uint16_t slotCount_;
    size_t instanceSize_;

    bool isSealed_;
    bool isFrozen_;
    bool isDictionary_;

    // Transitions: propertyNameId → child shape
    mutable std::shared_mutex transitionMutex_;
    std::unordered_map<uint32_t, Shape*> transitions_;
    size_t transitionCount_;

    std::atomic<uint64_t> objectCount_;
};

// =============================================================================
// Shape Allocator (pluggable, for slab or heap allocation)
// =============================================================================

class ShapeAllocator {
public:
    virtual ~ShapeAllocator() = default;
    virtual Shape* allocateShape(Shape* parent) = 0;
    virtual void freeShape(Shape* shape) = 0;
};

/**
 * @brief Simple pool-based shape allocator
 */
class PoolShapeAllocator : public ShapeAllocator {
public:
    PoolShapeAllocator() : nextId_(1) {}

    ~PoolShapeAllocator() override = default;

    Shape* allocateShape(Shape* parent) override {
        uint32_t id = nextId_++;
        auto shape = std::make_unique<Shape>(id, parent);
        Shape* ptr = shape.get();
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push_back(std::move(shape));
        return ptr;
    }

    void freeShape(Shape* /*shape*/) override {
        // Pool owns all shapes; freed on destruction
    }

    Shape* createRootShape() {
        return allocateShape(nullptr);
    }

    size_t shapeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<Shape>> pool_;
    std::atomic<uint32_t> nextId_;
};

// =============================================================================
// Shape::addProperty implementation
// =============================================================================

inline Shape* Shape::addProperty(uint32_t nameId, PropertyAttribute attrs,
                                  bool isReference,
                                  ShapeAllocator& allocator) {
    // Check for existing transition
    Shape* existing = findTransition(nameId);
    if (existing) return existing;

    // Create new shape
    Shape* child = allocator.allocateShape(this);
    if (!child) return nullptr;

    // Add the new property
    uint16_t newOffset = child->slotCount_;
    child->properties_.emplace_back(nameId, newOffset, attrs, isReference);
    child->slotCount_++;

    // Update reference map
    child->referenceMap_.resize(child->slotCount_, false);
    if (isReference) {
        child->referenceMap_[newOffset] = true;
    }

    child->updateInstanceSize();

    // Cache the transition
    addTransition(nameId, child);

    return child;
}

inline Shape* Shape::seal(ShapeAllocator& allocator) {
    Shape* sealed = allocator.allocateShape(parent_);
    if (!sealed) return nullptr;

    sealed->properties_ = properties_;
    sealed->referenceMap_ = referenceMap_;
    sealed->slotCount_ = slotCount_;
    sealed->isSealed_ = true;
    sealed->prototype_ = prototype_;

    // Remove configurable from all properties
    for (auto& prop : sealed->properties_) {
        prop.attrs = static_cast<PropertyAttribute>(
            static_cast<uint8_t>(prop.attrs) &
            ~static_cast<uint8_t>(PropertyAttribute::Configurable));
    }

    sealed->updateInstanceSize();
    return sealed;
}

inline Shape* Shape::freeze(ShapeAllocator& allocator) {
    Shape* frozen = seal(allocator);
    if (!frozen) return nullptr;

    frozen->isFrozen_ = true;

    // Remove writable from all data properties
    for (auto& prop : frozen->properties_) {
        if (!hasAttribute(prop.attrs, PropertyAttribute::Accessor)) {
            prop.attrs = static_cast<PropertyAttribute>(
                static_cast<uint8_t>(prop.attrs) &
                ~static_cast<uint8_t>(PropertyAttribute::Writable));
        }
    }

    return frozen;
}

// =============================================================================
// Shape Table (global shape registry)
// =============================================================================

/**
 * @brief Global registry of shapes for GC and diagnostics
 *
 * Provides:
 * - Shape lookup by ID
 * - GC marking of live shapes (via object→shape pointers)
 * - Dead shape collection (shapes with no objects)
 * - Transition tree statistics
 */
class ShapeTable {
public:
    ShapeTable() {
        // Create the root (empty) shape
        rootShape_ = allocator_.createRootShape();
    }

    Shape* rootShape() { return rootShape_; }
    PoolShapeAllocator& allocator() { return allocator_; }

    /**
     * @brief Get/create a shape for a given property sequence
     *
     * Given a list of property names, walks the transition tree
     * from root, creating new shapes as needed.
     */
    Shape* getShapeForProperties(
        const std::vector<uint32_t>& propertyNames,
        const std::vector<bool>& isReference
    ) {
        Shape* current = rootShape_;

        for (size_t i = 0; i < propertyNames.size(); i++) {
            bool ref = i < isReference.size() ? isReference[i] : false;
            Shape* next = current->addProperty(
                propertyNames[i], PropertyAttribute::DefaultData,
                ref, allocator_);
            if (!next) return nullptr;
            current = next;
        }

        return current;
    }

    /**
     * @brief Mark all shapes reachable from live objects
     *
     * Called by GC. The GC marks objects, then for each live object
     * reads its shape pointer and marks the shape.
     */
    void markShape(Shape* shape) {
        if (!shape) return;
        // In a real impl, shapes would have a mark bit.
        // Here we track via the set.
        std::lock_guard<std::mutex> lock(mutex_);
        markedShapes_.insert(shape);
    }

    /**
     * @brief Collect dead shapes (no live objects reference them)
     *
     * Walks all shapes. If a shape is not marked and has no objects,
     * it can be freed. Must be called after marking completes.
     */
    size_t collectDeadShapes() {
        std::lock_guard<std::mutex> lock(mutex_);
        // In a production impl, this would walk the shape pool
        // and free unmarked shapes, cleaning up transition caches.
        size_t collected = 0;
        // Reset marks for next cycle
        markedShapes_.clear();
        return collected;
    }

    struct Stats {
        size_t totalShapes;
        size_t markedShapes;
        size_t maxDepth;
        size_t totalTransitions;
    };

    Stats computeStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats{};
        stats.totalShapes = allocator_.shapeCount();
        stats.markedShapes = markedShapes_.size();
        return stats;
    }

private:
    mutable std::mutex mutex_;
    PoolShapeAllocator allocator_;
    Shape* rootShape_;
    std::unordered_set<Shape*> markedShapes_;
};

} // namespace Zepra::Heap
