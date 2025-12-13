#pragma once

/**
 * @file inspector.hpp
 * @brief DevTools Inspector panel - DOM/CSS inspection
 */

#include "../config.hpp"
#include "../runtime/value.hpp"
#include "../runtime/object.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Zepra::Debug {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief Property descriptor for object inspection
 */
struct PropertyDescriptor {
    std::string name;
    Value value;
    bool writable = true;
    bool enumerable = true;
    bool configurable = true;
    bool isGetter = false;
    bool isSetter = false;
};

/**
 * @brief Object preview for DevTools display
 */
struct ObjectPreview {
    std::string type;           // "object", "array", "function", etc.
    std::string subtype;        // "null", "regexp", "date", etc.
    std::string className;      // Constructor name
    std::string description;    // Preview text
    bool overflow = false;      // True if truncated
    std::vector<PropertyDescriptor> properties;
};

/**
 * @brief Remote object reference for DevTools
 */
struct RemoteObject {
    std::string objectId;       // Unique ID for object reference
    std::string type;
    std::string subtype;
    std::string className;
    Value value;
    std::string description;
    ObjectPreview preview;
    
    bool isObject() const { return type == "object" && !objectId.empty(); }
};

/**
 * @brief Inspector for runtime object inspection
 */
class Inspector {
public:
    /**
     * @brief Wrap a Value as RemoteObject for DevTools
     */
    static RemoteObject wrapValue(const Value& value, bool generatePreview = true);
    
    /**
     * @brief Get properties of an object
     */
    static std::vector<PropertyDescriptor> getProperties(Object* obj, bool ownOnly = false);
    
    /**
     * @brief Get internal properties (e.g., [[Prototype]])
     */
    static std::vector<PropertyDescriptor> getInternalProperties(Object* obj);
    
    /**
     * @brief Format value for display
     */
    static std::string formatValue(const Value& value, int maxDepth = 2);
    
    /**
     * @brief Get object by ID
     */
    static Object* getObjectById(const std::string& objectId);
    
    /**
     * @brief Release object reference
     */
    static void releaseObject(const std::string& objectId);
    
    /**
     * @brief Release all object references
     */
    static void releaseAllObjects();
    
private:
    static std::string generateObjectId();
    static std::string getTypeName(const Value& value);
    static ObjectPreview createPreview(Object* obj, int maxProps = 5);
};

/**
 * @brief CSS computed style for inspection
 */
struct CSSProperty {
    std::string name;
    std::string value;
    bool important = false;
    std::string source;  // "inline", "stylesheet", "inherited"
};

/**
 * @brief DOM node for inspection
 */
struct InspectedNode {
    int nodeId;
    std::string nodeType;
    std::string nodeName;
    std::string nodeValue;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<int> childNodeIds;
    
    // CSS
    std::vector<CSSProperty> computedStyle;
    std::vector<CSSProperty> inlineStyle;
};

/**
 * @brief DOM Inspector for element inspection
 */
class DOMInspector {
public:
    /**
     * @brief Get DOM tree starting from node
     */
    static std::vector<InspectedNode> getDocument(int depth = 2);
    
    /**
     * @brief Get node by ID
     */
    static InspectedNode getNode(int nodeId);
    
    /**
     * @brief Get computed style for node
     */
    static std::vector<CSSProperty> getComputedStyle(int nodeId);
    
    /**
     * @brief Highlight node in viewport
     */
    static void highlightNode(int nodeId);
    
    /**
     * @brief Remove highlight
     */
    static void hideHighlight();
    
    /**
     * @brief Get node at position (for element picker)
     */
    static int getNodeAtPosition(int x, int y);
    
    /**
     * @brief Set attribute value
     */
    static void setAttribute(int nodeId, const std::string& name, const std::string& value);
    
    /**
     * @brief Set inner HTML
     */
    static void setInnerHTML(int nodeId, const std::string& html);
};

} // namespace Zepra::Debug
