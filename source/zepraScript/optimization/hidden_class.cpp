/**
 * @file hidden_class.cpp
 * @brief V8-style hidden classes (shapes/maps) for fast property access
 *
 * Every JS object has a hidden class (shape) describing its property layout.
 * Objects with the same property sequence share a hidden class, enabling:
 * - Fixed-offset property lookups (no hash table lookup needed)
 * - Inline cache hits on property access
 * - Fast prototype chain traversal
 *
 * Transitions: When a property is added, the object transitions from
 * one hidden class to another. This forms a transition tree.
 *
 * Ref: V8 Maps, SpiderMonkey Shapes, JSC Structures
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <cstdio>

namespace Zepra::Optimization {

// =============================================================================
// Property Descriptor (slot metadata)
// =============================================================================

struct PropertySlot {
    std::string name;
    uint32_t offset;     // Byte offset in object storage
    uint8_t attributes;  // writable, enumerable, configurable

    static constexpr uint8_t Writable     = 0x01;
    static constexpr uint8_t Enumerable   = 0x02;
    static constexpr uint8_t Configurable = 0x04;
    static constexpr uint8_t DefaultAttrs = Writable | Enumerable | Configurable;
};

// =============================================================================
// Hidden Class
// =============================================================================

class HiddenClass {
public:
    using TransitionMap = std::unordered_map<std::string, HiddenClass*>;

    HiddenClass()
        : id_(nextId_++)
        , parent_(nullptr)
        , propertyCount_(0)
        , inlineCapacity_(4)
        , overflowOffset_(0)
        , prototype_(nullptr)
        , frozen_(false)
        , sealed_(false) {}

    uint32_t id() const { return id_; }
    uint32_t propertyCount() const { return propertyCount_; }
    bool isFrozen() const { return frozen_; }
    bool isSealed() const { return sealed_; }

    // =========================================================================
    // Property lookup
    // =========================================================================

    /**
     * Find the offset of a property by name.
     * Returns -1 if not found.
     */
    int32_t findProperty(const std::string& name) const {
        for (const auto& slot : slots_) {
            if (slot.name == name) {
                return static_cast<int32_t>(slot.offset);
            }
        }
        return -1;
    }

    const PropertySlot* getSlot(const std::string& name) const {
        for (const auto& slot : slots_) {
            if (slot.name == name) return &slot;
        }
        return nullptr;
    }

    const std::vector<PropertySlot>& slots() const { return slots_; }

    // =========================================================================
    // Transitions
    // =========================================================================

    /**
     * Add a property and transition to a new hidden class.
     * If a transition already exists for this property name, reuse it.
     */
    HiddenClass* addProperty(const std::string& name,
                              uint8_t attrs = PropertySlot::DefaultAttrs) {
        if (frozen_) return nullptr;

        // Check existing transitions
        auto it = transitions_.find(name);
        if (it != transitions_.end()) {
            return it->second;
        }

        // Create new hidden class
        auto* newClass = new HiddenClass();
        newClass->parent_ = this;
        newClass->slots_ = this->slots_;
        newClass->propertyCount_ = this->propertyCount_ + 1;
        newClass->inlineCapacity_ = this->inlineCapacity_;
        newClass->overflowOffset_ = this->overflowOffset_;
        newClass->prototype_ = this->prototype_;

        // Add new property slot
        PropertySlot slot;
        slot.name = name;
        slot.offset = this->propertyCount_; // Sequential offsets
        slot.attributes = attrs;
        newClass->slots_.push_back(slot);

        // If we exceeded inline capacity, start overflow
        if (newClass->propertyCount_ > newClass->inlineCapacity_) {
            newClass->overflowOffset_ =
                newClass->propertyCount_ - newClass->inlineCapacity_;
        }

        // Cache the transition
        transitions_[name] = newClass;
        ownedChildren_.emplace_back(newClass);

        return newClass;
    }

    /**
     * Delete a property — transitions to a new class without it.
     * This is expensive (creates a new branch).
     */
    HiddenClass* deleteProperty(const std::string& name) {
        int32_t offset = findProperty(name);
        if (offset < 0) return this;

        auto* newClass = new HiddenClass();
        newClass->parent_ = this;
        newClass->prototype_ = this->prototype_;
        newClass->inlineCapacity_ = this->inlineCapacity_;

        // Copy all slots except the deleted one, reindexing offsets
        uint32_t newOffset = 0;
        for (const auto& slot : slots_) {
            if (slot.name == name) continue;
            PropertySlot ns = slot;
            ns.offset = newOffset++;
            newClass->slots_.push_back(ns);
        }
        newClass->propertyCount_ = newOffset;

        ownedChildren_.emplace_back(newClass);
        return newClass;
    }

    // =========================================================================
    // Prototype
    // =========================================================================

    void setPrototype(HiddenClass* proto) { prototype_ = proto; }
    HiddenClass* prototype() const { return prototype_; }

    // =========================================================================
    // Freeze/Seal
    // =========================================================================

    HiddenClass* freeze() {
        if (frozen_) return this;
        auto* newClass = new HiddenClass();
        newClass->parent_ = this;
        newClass->slots_ = this->slots_;
        newClass->propertyCount_ = this->propertyCount_;
        newClass->inlineCapacity_ = this->inlineCapacity_;
        newClass->overflowOffset_ = this->overflowOffset_;
        newClass->prototype_ = this->prototype_;
        newClass->frozen_ = true;
        newClass->sealed_ = true;
        for (auto& slot : newClass->slots_) {
            slot.attributes &= ~(PropertySlot::Writable | PropertySlot::Configurable);
        }
        ownedChildren_.emplace_back(newClass);
        return newClass;
    }

    HiddenClass* seal() {
        if (sealed_) return this;
        auto* newClass = new HiddenClass();
        newClass->parent_ = this;
        newClass->slots_ = this->slots_;
        newClass->propertyCount_ = this->propertyCount_;
        newClass->inlineCapacity_ = this->inlineCapacity_;
        newClass->overflowOffset_ = this->overflowOffset_;
        newClass->prototype_ = this->prototype_;
        newClass->sealed_ = true;
        for (auto& slot : newClass->slots_) {
            slot.attributes &= ~PropertySlot::Configurable;
        }
        ownedChildren_.emplace_back(newClass);
        return newClass;
    }

    // =========================================================================
    // Statistics
    // =========================================================================

    size_t transitionCount() const { return transitions_.size(); }
    size_t treeDepth() const {
        size_t depth = 0;
        const HiddenClass* c = this;
        while (c->parent_) { c = c->parent_; depth++; }
        return depth;
    }

    // =========================================================================
    // Debug
    // =========================================================================

    void dump() const {
        printf("HiddenClass #%u (%u props, %zu transitions, depth %zu)\n",
               id_, propertyCount_, transitions_.size(), treeDepth());
        for (const auto& slot : slots_) {
            printf("  [%u] %s (attrs: %s%s%s)\n",
                   slot.offset, slot.name.c_str(),
                   (slot.attributes & PropertySlot::Writable) ? "W" : "",
                   (slot.attributes & PropertySlot::Enumerable) ? "E" : "",
                   (slot.attributes & PropertySlot::Configurable) ? "C" : "");
        }
    }

private:
    uint32_t id_;
    HiddenClass* parent_;
    uint32_t propertyCount_;
    uint32_t inlineCapacity_;
    uint32_t overflowOffset_;
    HiddenClass* prototype_;
    bool frozen_;
    bool sealed_;

    std::vector<PropertySlot> slots_;
    TransitionMap transitions_;
    std::vector<std::unique_ptr<HiddenClass>> ownedChildren_;

    static uint32_t nextId_;
};

uint32_t HiddenClass::nextId_ = 0;

// =============================================================================
// Hidden Class Table (global registry)
// =============================================================================

class HiddenClassTable {
public:
    static HiddenClassTable& instance() {
        static HiddenClassTable inst;
        return inst;
    }

    /**
     * Get or create the root hidden class (empty object shape).
     */
    HiddenClass* emptyClass() {
        if (!emptyClass_) {
            emptyClass_ = std::make_unique<HiddenClass>();
        }
        return emptyClass_.get();
    }

    /**
     * Get statistics about the transition tree.
     */
    struct Stats {
        size_t totalClasses = 0;
        size_t maxDepth = 0;
        size_t totalTransitions = 0;
    };

    Stats getStats() const {
        Stats s;
        if (emptyClass_) {
            collectStats(emptyClass_.get(), s, 0);
        }
        return s;
    }

private:
    HiddenClassTable() = default;

    void collectStats(const HiddenClass* cls, Stats& s, size_t depth) const {
        s.totalClasses++;
        s.totalTransitions += cls->transitionCount();
        if (depth > s.maxDepth) s.maxDepth = depth;
    }

    std::unique_ptr<HiddenClass> emptyClass_;
};

} // namespace Zepra::Optimization
