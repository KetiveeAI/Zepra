/**
 * @file Shape.h
 * @brief Hidden class / shape implementation for property access optimization
 * 
 * Implements:
 * - Shape transitions
 * - Property descriptor storage
 * - Inline cache integration
 * - Shape tree management
 * 
 * Based on V8 Maps and JSC Structures
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace Zepra::Runtime {

// Forward declarations
class Object;
class Shape;

// =============================================================================
// Property Attributes
// =============================================================================

enum class PropertyAttributes : uint8_t {
    None        = 0,
    Writable    = 1 << 0,
    Enumerable  = 1 << 1,
    Configurable = 1 << 2,
    
    // Accessor property
    Accessor    = 1 << 3,
    
    // Default for data property
    Default     = Writable | Enumerable | Configurable,
    
    // Read-only, non-configurable
    ReadOnly    = Enumerable
};

inline PropertyAttributes operator|(PropertyAttributes a, PropertyAttributes b) {
    return static_cast<PropertyAttributes>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasAttribute(PropertyAttributes attrs, PropertyAttributes flag) {
    return (static_cast<uint8_t>(attrs) & static_cast<uint8_t>(flag)) != 0;
}

// =============================================================================
// Property Descriptor
// =============================================================================

struct PropertyDescriptor {
    uint32_t offset;            // Offset in property storage
    PropertyAttributes attributes;
    
    bool isWritable() const { return hasAttribute(attributes, PropertyAttributes::Writable); }
    bool isEnumerable() const { return hasAttribute(attributes, PropertyAttributes::Enumerable); }
    bool isConfigurable() const { return hasAttribute(attributes, PropertyAttributes::Configurable); }
    bool isAccessor() const { return hasAttribute(attributes, PropertyAttributes::Accessor); }
};

// =============================================================================
// Shape Transition
// =============================================================================

struct ShapeTransition {
    std::string propertyName;
    PropertyAttributes attributes;
    Shape* target = nullptr;
};

// =============================================================================
// Shape
// =============================================================================

/**
 * @brief Hidden class for object property layout
 * 
 * Shapes form a tree where each transition adds a property.
 * Objects with the same shape have the same property layout.
 */
class Shape {
public:
    using ShapeId = uint32_t;
    static constexpr ShapeId INVALID_SHAPE_ID = UINT32_MAX;
    
    Shape();
    explicit Shape(Shape* parent, const std::string& transitionKey, 
                   PropertyDescriptor descriptor);
    
    // =========================================================================
    // Identity
    // =========================================================================
    
    ShapeId id() const { return id_; }
    
    // =========================================================================
    // Parent / Transitions
    // =========================================================================
    
    Shape* parent() const { return parent_; }
    bool isRoot() const { return parent_ == nullptr; }
    
    /**
     * @brief Get or create transition for adding a property
     */
    Shape* getTransition(const std::string& name, PropertyAttributes attrs);
    
    /**
     * @brief Check if transition exists
     */
    bool hasTransition(const std::string& name) const;
    
    // =========================================================================
    // Properties
    // =========================================================================
    
    /**
     * @brief Get property descriptor by name
     */
    const PropertyDescriptor* getProperty(const std::string& name) const;
    
    /**
     * @brief Get property descriptor by offset
     */
    const PropertyDescriptor* getPropertyByOffset(uint32_t offset) const;
    
    /**
     * @brief Number of properties
     */
    size_t propertyCount() const { return properties_.size(); }
    
    /**
     * @brief Get all property names
     */
    std::vector<std::string> propertyNames() const;
    
    /**
     * @brief Get enumerable property names
     */
    std::vector<std::string> enumerablePropertyNames() const;
    
    // =========================================================================
    // Inline Cache Integration
    // =========================================================================
    
    /**
     * @brief Get property offset for IC
     * @return Offset or -1 if not found
     */
    int32_t getOffset(const std::string& name) const;
    
    /**
     * @brief Check shape compatibility for IC
     */
    bool isCompatibleWith(const Shape* other) const;
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    size_t depth() const;
    size_t transitionCount() const { return transitions_.size(); }
    
    // =========================================================================
    // Factory
    // =========================================================================
    
    static Shape* createRootShape();
    
private:
    ShapeId id_;
    Shape* parent_ = nullptr;
    std::string transitionKey_;
    
    // Property table (inherited + own)
    std::unordered_map<std::string, PropertyDescriptor> properties_;
    
    // Transitions to child shapes
    std::unordered_map<std::string, std::unique_ptr<Shape>> transitions_;
    
    // Global shape counter
    static std::atomic<ShapeId> nextId_;
    
    Shape* createTransition(const std::string& name, PropertyAttributes attrs);
};

// =============================================================================
// Shape Table (global registry)
// =============================================================================

class ShapeTable {
public:
    ShapeTable();
    
    /**
     * @brief Get root shape for new objects
     */
    Shape* rootShape() { return rootShape_.get(); }
    
    /**
     * @brief Get shape by ID
     */
    Shape* getShape(Shape::ShapeId id);
    
    /**
     * @brief Register a shape
     */
    void registerShape(Shape* shape);
    
    /**
     * @brief Statistics
     */
    struct Stats {
        size_t totalShapes = 0;
        size_t maxDepth = 0;
        size_t totalTransitions = 0;
    };
    
    Stats getStats() const;
    
private:
    std::unique_ptr<Shape> rootShape_;
    std::unordered_map<Shape::ShapeId, Shape*> shapeIndex_;
};

// =============================================================================
// Shape Utilities
// =============================================================================

namespace ShapeUtils {

/**
 * @brief Find common ancestor shape
 */
Shape* findCommonAncestor(Shape* a, Shape* b);

/**
 * @brief Compute transition path between shapes
 */
std::vector<std::string> computeTransitionPath(Shape* from, Shape* to);

} // namespace ShapeUtils

} // namespace Zepra::Runtime
