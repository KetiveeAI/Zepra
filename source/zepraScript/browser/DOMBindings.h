/**
 * @file DOMBindings.h
 * @brief C++ ↔ JavaScript DOM Interop
 * 
 * WebIDL-style bindings for exposing C++ DOM to JS:
 * - DOMClass: Wrap C++ classes
 * - DOMMethod: Expose methods
 * - DOMProperty: Getters/setters
 * - DOMEventTarget: Event dispatching
 */

#pragma once

#include "../core/EmbedderAPI.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace Zepra::DOM {

// =============================================================================
// DOM Wrapper Base
// =============================================================================

/**
 * @brief Base class for all DOM wrappers
 */
class DOMWrapper {
public:
    virtual ~DOMWrapper() = default;
    
    // Get the wrapped C++ object
    virtual void* GetNativeObject() const = 0;
    
    // Get wrapper for a native object
    template<typename T>
    static DOMWrapper* Wrap(T* native);
    
    // Get native from wrapper
    template<typename T>
    static T* Unwrap(DOMWrapper* wrapper) {
        return static_cast<T*>(wrapper->GetNativeObject());
    }
};

// =============================================================================
// Property Descriptor
// =============================================================================

enum class PropertyFlags : uint8_t {
    None = 0,
    ReadOnly = 1 << 0,
    DontEnum = 1 << 1,
    DontDelete = 1 << 2,
    Accessor = 1 << 3
};

inline PropertyFlags operator|(PropertyFlags a, PropertyFlags b) {
    return static_cast<PropertyFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

struct DOMPropertyDescriptor {
    std::string name;
    PropertyFlags flags = PropertyFlags::None;
    
    // For data properties
    std::function<ZebraValue(void*)> getter;
    std::function<void(void*, const ZebraValue&)> setter;
    
    bool isReadOnly() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(PropertyFlags::ReadOnly)) != 0;
    }
};

// =============================================================================
// Method Descriptor
// =============================================================================

struct DOMMethodDescriptor {
    std::string name;
    int length = 0;  // Expected argument count
    
    std::function<ZebraValue(void*, const std::vector<ZebraValue>&)> callback;
};

// =============================================================================
// DOM Class Definition
// =============================================================================

/**
 * @brief Defines a JavaScript class backed by C++
 */
class DOMClassDefinition {
public:
    DOMClassDefinition(const std::string& className)
        : className_(className) {}
    
    const std::string& ClassName() const { return className_; }
    
    // Add property
    template<typename T, typename R>
    DOMClassDefinition& Property(const std::string& name,
                                  R (T::*getter)() const,
                                  void (T::*setter)(R) = nullptr) {
        DOMPropertyDescriptor desc;
        desc.name = name;
        desc.getter = [getter](void* obj) -> ZebraValue {
            T* self = static_cast<T*>(obj);
            return ToJS((self->*getter)());
        };
        
        if (setter) {
            desc.setter = [setter](void* obj, const ZebraValue& val) {
                T* self = static_cast<T*>(obj);
                (self->*setter)(FromJS<R>(val));
            };
        } else {
            desc.flags = PropertyFlags::ReadOnly;
        }
        
        properties_.push_back(std::move(desc));
        return *this;
    }
    
    // Add method
    template<typename T, typename R, typename... Args>
    DOMClassDefinition& Method(const std::string& name,
                                R (T::*method)(Args...)) {
        DOMMethodDescriptor desc;
        desc.name = name;
        desc.length = sizeof...(Args);
        desc.callback = [method](void* obj, const std::vector<ZebraValue>& args) 
            -> ZebraValue {
            T* self = static_cast<T*>(obj);
            return CallMethod(self, method, args, std::index_sequence_for<Args...>{});
        };
        
        methods_.push_back(std::move(desc));
        return *this;
    }
    
    // Add static method
    template<typename R, typename... Args>
    DOMClassDefinition& StaticMethod(const std::string& name,
                                      R (*method)(Args...)) {
        DOMMethodDescriptor desc;
        desc.name = name;
        desc.length = sizeof...(Args);
        desc.callback = [method](void*, const std::vector<ZebraValue>& args) 
            -> ZebraValue {
            return CallStaticMethod(method, args, std::index_sequence_for<Args...>{});
        };
        
        staticMethods_.push_back(std::move(desc));
        return *this;
    }
    
    const std::vector<DOMPropertyDescriptor>& Properties() const { return properties_; }
    const std::vector<DOMMethodDescriptor>& Methods() const { return methods_; }
    const std::vector<DOMMethodDescriptor>& StaticMethods() const { return staticMethods_; }
    
private:
    template<typename R>
    static ZebraValue ToJS(R value);
    
    template<typename R>
    static R FromJS(const ZebraValue& value);
    
    template<typename T, typename R, typename... Args, size_t... Is>
    static ZebraValue CallMethod(T* self, R (T::*method)(Args...),
                                  const std::vector<ZebraValue>& args,
                                  std::index_sequence<Is...>) {
        return ToJS((self->*method)(FromJS<Args>(args[Is])...));
    }
    
    template<typename R, typename... Args, size_t... Is>
    static ZebraValue CallStaticMethod(R (*method)(Args...),
                                        const std::vector<ZebraValue>& args,
                                        std::index_sequence<Is...>) {
        return ToJS(method(FromJS<Args>(args[Is])...));
    }
    
    std::string className_;
    std::vector<DOMPropertyDescriptor> properties_;
    std::vector<DOMMethodDescriptor> methods_;
    std::vector<DOMMethodDescriptor> staticMethods_;
};

// =============================================================================
// DOM Event Target
// =============================================================================

/**
 * @brief Base class for event-dispatching DOM objects
 */
class DOMEventTarget {
public:
    virtual ~DOMEventTarget() = default;
    
    // Add event listener
    void AddEventListener(const std::string& type, 
                          std::function<void(const ZebraValue&)> listener,
                          bool capture = false) {
        auto& list = capture ? captureListeners_[type] : bubbleListeners_[type];
        list.push_back(std::move(listener));
    }
    
    // Remove event listener
    void RemoveEventListener(const std::string& type, bool capture = false) {
        auto& list = capture ? captureListeners_[type] : bubbleListeners_[type];
        list.clear();
    }
    
    // Dispatch event
    bool DispatchEvent(const std::string& type, const ZebraValue& event) {
        // Capture phase
        auto capIt = captureListeners_.find(type);
        if (capIt != captureListeners_.end()) {
            for (auto& listener : capIt->second) {
                listener(event);
            }
        }
        
        // Bubble phase
        auto bubIt = bubbleListeners_.find(type);
        if (bubIt != bubbleListeners_.end()) {
            for (auto& listener : bubIt->second) {
                listener(event);
            }
        }
        
        return true;
    }
    
private:
    std::unordered_map<std::string, 
        std::vector<std::function<void(const ZebraValue&)>>> captureListeners_;
    std::unordered_map<std::string, 
        std::vector<std::function<void(const ZebraValue&)>>> bubbleListeners_;
};

// =============================================================================
// DOM Class Registry
// =============================================================================

/**
 * @brief Global registry of DOM classes
 */
class DOMClassRegistry {
public:
    static DOMClassRegistry& Instance() {
        static DOMClassRegistry registry;
        return registry;
    }
    
    // Register a class
    void Register(std::type_index type, DOMClassDefinition def) {
        definitions_[type] = std::move(def);
    }
    
    // Get definition
    const DOMClassDefinition* Get(std::type_index type) const {
        auto it = definitions_.find(type);
        return it != definitions_.end() ? &it->second : nullptr;
    }
    
    // Install all classes to context
    void InstallToContext(ZebraContext* ctx);
    
private:
    DOMClassRegistry() = default;
    std::unordered_map<std::type_index, DOMClassDefinition> definitions_;
};

// =============================================================================
// Registration Macro
// =============================================================================

#define ZEPRA_REGISTER_DOM_CLASS(CppClass, JsName) \
    static bool _registered_##CppClass = []() { \
        Zepra::DOM::DOMClassRegistry::Instance().Register( \
            std::type_index(typeid(CppClass)), \
            Zepra::DOM::DOMClassDefinition(JsName) \
        ); \
        return true; \
    }()

// =============================================================================
// NodeList
// =============================================================================

/**
 * @brief Collection of DOM nodes (read-only)
 */
class NodeList {
public:
    NodeList() = default;
    
    size_t length() const { return nodes_.size(); }
    
    DOMWrapper* item(size_t index) const {
        return index < nodes_.size() ? nodes_[index] : nullptr;
    }
    
    DOMWrapper* operator[](size_t index) const { return item(index); }
    
    // Iterator support
    auto begin() const { return nodes_.begin(); }
    auto end() const { return nodes_.end(); }
    
    // forEach callback type
    using ForEachCallback = std::function<void(DOMWrapper*, size_t, const NodeList&)>;
    
    void forEach(ForEachCallback callback) const {
        for (size_t i = 0; i < nodes_.size(); i++) {
            callback(nodes_[i], i, *this);
        }
    }
    
    // Internal: add node
    void add(DOMWrapper* node) { nodes_.push_back(node); }
    
private:
    std::vector<DOMWrapper*> nodes_;
};

// =============================================================================
// HTMLCollection
// =============================================================================

/**
 * @brief Live collection of HTML elements
 */
class HTMLCollection {
public:
    HTMLCollection() = default;
    
    size_t length() const { return elements_.size(); }
    
    DOMWrapper* item(size_t index) const {
        return index < elements_.size() ? elements_[index] : nullptr;
    }
    
    DOMWrapper* namedItem(const std::string& name) const {
        auto it = namedItems_.find(name);
        return it != namedItems_.end() ? it->second : nullptr;
    }
    
    // Internal
    void add(DOMWrapper* elem, const std::string& id = "") {
        elements_.push_back(elem);
        if (!id.empty()) {
            namedItems_[id] = elem;
        }
    }
    
private:
    std::vector<DOMWrapper*> elements_;
    std::unordered_map<std::string, DOMWrapper*> namedItems_;
};

// =============================================================================
// MutationRecord
// =============================================================================

/**
 * @brief Record of a DOM mutation
 */
struct MutationRecord {
    enum class Type { Attributes, CharacterData, ChildList };
    
    Type type;
    DOMWrapper* target = nullptr;
    NodeList addedNodes;
    NodeList removedNodes;
    DOMWrapper* previousSibling = nullptr;
    DOMWrapper* nextSibling = nullptr;
    std::string attributeName;
    std::string attributeNamespace;
    std::string oldValue;
};

// =============================================================================
// MutationObserverInit
// =============================================================================

/**
 * @brief Options for MutationObserver.observe()
 */
struct MutationObserverInit {
    bool childList = false;
    bool attributes = false;
    bool characterData = false;
    bool subtree = false;
    bool attributeOldValue = false;
    bool characterDataOldValue = false;
    std::vector<std::string> attributeFilter;
};

// =============================================================================
// MutationObserver
// =============================================================================

/**
 * @brief Observes DOM mutations
 */
class MutationObserver {
public:
    using Callback = std::function<void(const std::vector<MutationRecord>&, MutationObserver*)>;
    
    explicit MutationObserver(Callback callback);
    ~MutationObserver();
    
    /**
     * @brief Start observing a target node
     */
    void observe(DOMWrapper* target, const MutationObserverInit& options);
    
    /**
     * @brief Stop observing all targets
     */
    void disconnect();
    
    /**
     * @brief Get pending records and clear the queue
     */
    std::vector<MutationRecord> takeRecords();
    
    // Internal: notify of mutation
    void notifyMutation(const MutationRecord& record);
    
private:
    Callback callback_;
    std::vector<std::pair<DOMWrapper*, MutationObserverInit>> targets_;
    std::vector<MutationRecord> pendingRecords_;
};

// =============================================================================
// IntersectionObserverEntry
// =============================================================================

/**
 * @brief Entry describing intersection state
 */
struct IntersectionObserverEntry {
    double time;
    DOMWrapper* target = nullptr;
    
    // Bounding rectangles
    struct Rect { double x, y, width, height, top, right, bottom, left; };
    Rect boundingClientRect;
    Rect intersectionRect;
    Rect rootBounds;
    
    double intersectionRatio = 0.0;
    bool isIntersecting = false;
    bool isVisible = false;
};

// =============================================================================
// IntersectionObserverInit
// =============================================================================

struct IntersectionObserverInit {
    DOMWrapper* root = nullptr;  // null = viewport
    std::string rootMargin = "0px";
    std::vector<double> threshold = {0.0};
};

// =============================================================================
// IntersectionObserver
// =============================================================================

/**
 * @brief Observes element visibility in viewport
 */
class IntersectionObserver {
public:
    using Callback = std::function<void(const std::vector<IntersectionObserverEntry>&, IntersectionObserver*)>;
    
    explicit IntersectionObserver(Callback callback, const IntersectionObserverInit& options = {});
    ~IntersectionObserver();
    
    void observe(DOMWrapper* target);
    void unobserve(DOMWrapper* target);
    void disconnect();
    std::vector<IntersectionObserverEntry> takeRecords();
    
    // Properties
    DOMWrapper* root() const { return options_.root; }
    const std::string& rootMargin() const { return options_.rootMargin; }
    const std::vector<double>& thresholds() const { return options_.threshold; }
    
private:
    Callback callback_;
    IntersectionObserverInit options_;
    std::vector<DOMWrapper*> targets_;
    std::vector<IntersectionObserverEntry> pendingEntries_;
};

// =============================================================================
// Query Selector API
// =============================================================================

/**
 * @brief CSS selector query interface
 */
class QuerySelector {
public:
    /**
     * @brief Find first matching element
     */
    static DOMWrapper* querySelector(DOMWrapper* root, const std::string& selector);
    
    /**
     * @brief Find all matching elements
     */
    static NodeList querySelectorAll(DOMWrapper* root, const std::string& selector);
    
    /**
     * @brief Check if element matches selector
     */
    static bool matches(DOMWrapper* element, const std::string& selector);
    
    /**
     * @brief Find closest ancestor matching selector
     */
    static DOMWrapper* closest(DOMWrapper* element, const std::string& selector);
    
private:
    // Simple selector parser (class, id, tag, attribute)
    struct Selector {
        std::string tag;
        std::string id;
        std::vector<std::string> classes;
        std::vector<std::pair<std::string, std::string>> attributes;
    };
    
    static Selector parseSelector(const std::string& selector);
    static bool matchesSimple(DOMWrapper* element, const Selector& sel);
};

} // namespace Zepra::DOM
