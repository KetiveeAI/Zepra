/**
 * @file inspector.cpp
 * @brief DevTools Inspector implementation
 */

#include "zeprascript/debug/inspector.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/function.hpp"
#include <sstream>
#include <iomanip>

namespace Zepra::Debug {

using Runtime::Object;
using Runtime::Function;

// Object ID management
static std::unordered_map<std::string, Object*> objectRegistry;
static int nextObjectId = 1;

std::string Inspector::generateObjectId() {
    return "obj_" + std::to_string(nextObjectId++);
}

std::string Inspector::getTypeName(const Value& value) {
    if (value.isUndefined()) return "undefined";
    if (value.isNull()) return "object";  // typeof null === "object"
    if (value.isBoolean()) return "boolean";
    if (value.isNumber()) return "number";
    if (value.isString()) return "string";
    if (value.isObject()) {
        Object* obj = value.asObject();
        if (obj->isFunction()) return "function";
        return "object";
    }
    return "undefined";
}

RemoteObject Inspector::wrapValue(const Value& value, bool generatePreview) {
    RemoteObject remote;
    remote.type = getTypeName(value);
    
    if (value.isUndefined()) {
        remote.description = "undefined";
    } else if (value.isNull()) {
        remote.subtype = "null";
        remote.description = "null";
    } else if (value.isBoolean()) {
        remote.value = value;
        remote.description = value.asBoolean() ? "true" : "false";
    } else if (value.isNumber()) {
        remote.value = value;
        std::ostringstream ss;
        ss << value.asNumber();
        remote.description = ss.str();
    } else if (value.isString()) {
        remote.value = value;
        remote.description = "\"string\"";  // Simplified
    } else if (value.isObject()) {
        Object* obj = value.asObject();
        remote.objectId = generateObjectId();
        objectRegistry[remote.objectId] = obj;
        
        if (obj->isArray()) {
            remote.subtype = "array";
            remote.className = "Array";
            remote.description = "Array";
        } else if (obj->isFunction()) {
            remote.className = "Function";
            Function* fn = static_cast<Function*>(obj);
            remote.description = "function " + fn->name() + "()";
        } else {
            remote.className = "Object";
            remote.description = "Object";
        }
        
        if (generatePreview) {
            remote.preview = createPreview(obj);
        }
    }
    
    return remote;
}

ObjectPreview Inspector::createPreview(Object* obj, int maxProps) {
    ObjectPreview preview;
    preview.type = "object";
    
    auto keys = obj->getOwnPropertyNames();
    int count = 0;
    for (const auto& key : keys) {
        if (count >= maxProps) {
            preview.overflow = true;
            break;
        }
        PropertyDescriptor prop;
        prop.name = key;
        prop.value = obj->get(key);
        preview.properties.push_back(prop);
        count++;
    }
    
    return preview;
}

std::vector<PropertyDescriptor> Inspector::getProperties(Object* obj, bool) {
    std::vector<PropertyDescriptor> props;
    
    auto keys = obj->getOwnPropertyNames();
    for (const auto& key : keys) {
        PropertyDescriptor prop;
        prop.name = key;
        prop.value = obj->get(key);
        prop.enumerable = true;
        prop.writable = true;
        prop.configurable = true;
        props.push_back(prop);
    }
    
    return props;
}

std::vector<PropertyDescriptor> Inspector::getInternalProperties(Object*) {
    std::vector<PropertyDescriptor> props;
    // TODO: Add internal properties when Object::getPrototype exists
    return props;
}

std::string Inspector::formatValue(const Value& value, int maxDepth) {
    if (maxDepth <= 0) return "...";
    
    if (value.isUndefined()) return "undefined";
    if (value.isNull()) return "null";
    if (value.isBoolean()) return value.asBoolean() ? "true" : "false";
    
    if (value.isNumber()) {
        std::ostringstream ss;
        ss << value.asNumber();
        return ss.str();
    }
    
    if (value.isString()) {
        return "\"string\"";  // Simplified
    }
    
    if (value.isObject()) {
        Object* obj = value.asObject();
        
        if (obj->isArray()) {
            return "[Array]";
        }
        
        if (obj->isFunction()) {
            Function* fn = static_cast<Function*>(obj);
            return "[Function: " + fn->name() + "]";
        }
        
        return "[Object]";
    }
    
    return "?";
}

Object* Inspector::getObjectById(const std::string& objectId) {
    auto it = objectRegistry.find(objectId);
    return it != objectRegistry.end() ? it->second : nullptr;
}

void Inspector::releaseObject(const std::string& objectId) {
    objectRegistry.erase(objectId);
}

void Inspector::releaseAllObjects() {
    objectRegistry.clear();
}

// =============================================================================
// DOMInspector (stubs)
// =============================================================================

std::vector<InspectedNode> DOMInspector::getDocument(int) { return {}; }
InspectedNode DOMInspector::getNode(int) { return {}; }
std::vector<CSSProperty> DOMInspector::getComputedStyle(int) { return {}; }
void DOMInspector::highlightNode(int) {}
void DOMInspector::hideHighlight() {}
int DOMInspector::getNodeAtPosition(int, int) { return -1; }
void DOMInspector::setAttribute(int, const std::string&, const std::string&) {}
void DOMInspector::setInnerHTML(int, const std::string&) {}

} // namespace Zepra::Debug
