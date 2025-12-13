#pragma once

/**
 * @file document.hpp
 * @brief JavaScript Document object for DOM manipulation
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

// Forward declarations
class Element;
class Window;

/**
 * @brief JavaScript Element object
 */
class Element : public Object {
public:
    Element(const std::string& tagName);
    
    // Properties
    const std::string& tagName() const { return tagName_; }
    const std::string& id() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    const std::string& className() const { return className_; }
    void setClassName(const std::string& cls) { className_ = cls; }
    
    const std::string& innerHTML() const { return innerHTML_; }
    void setInnerHTML(const std::string& html) { innerHTML_ = html; }
    
    const std::string& textContent() const { return textContent_; }
    void setTextContent(const std::string& text) { textContent_ = text; }
    
    // Attributes
    std::string getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    void removeAttribute(const std::string& name);
    bool hasAttribute(const std::string& name) const;
    
    // Style
    Value style() const { return style_; }
    
    // Tree navigation
    Element* parentElement() const { return parentElement_; }
    const std::vector<Element*>& children() const { return children_; }
    Element* firstChild() const;
    Element* lastChild() const;
    Element* nextSibling() const { return nextSibling_; }
    Element* previousSibling() const { return previousSibling_; }
    
    // Tree manipulation
    void appendChild(Element* child);
    void removeChild(Element* child);
    void insertBefore(Element* newChild, Element* refChild);
    Element* cloneNode(bool deep = false) const;
    
    // Query
    Element* querySelector(const std::string& selector) const;
    std::vector<Element*> querySelectorAll(const std::string& selector) const;
    Element* getElementById(const std::string& id) const;
    std::vector<Element*> getElementsByClassName(const std::string& className) const;
    std::vector<Element*> getElementsByTagName(const std::string& tagName) const;
    
    // Events
    void addEventListener(const std::string& type, Value callback);
    void removeEventListener(const std::string& type, Value callback);
    void dispatchEvent(const std::string& type, Value eventData);
    
private:
    std::string tagName_;
    std::string id_;
    std::string className_;
    std::string innerHTML_;
    std::string textContent_;
    std::unordered_map<std::string, std::string> attributes_;
    Value style_ = Value::undefined();
    
    Element* parentElement_ = nullptr;
    std::vector<Element*> children_;
    Element* nextSibling_ = nullptr;
    Element* previousSibling_ = nullptr;
    
    std::unordered_map<std::string, std::vector<Value>> eventListeners_;
};

/**
 * @brief JavaScript Document object
 */
class Document : public Object {
public:
    explicit Document(Window* window);
    
    // Document properties
    Element* documentElement() const { return documentElement_; }
    Element* body() const { return body_; }
    Element* head() const { return head_; }
    const std::string& title() const { return title_; }
    void setTitle(const std::string& title) { title_ = title; }
    
    // Element creation
    Element* createElement(const std::string& tagName);
    Element* createTextNode(const std::string& text);
    
    // Query methods
    Element* getElementById(const std::string& id);
    std::vector<Element*> getElementsByClassName(const std::string& className);
    std::vector<Element*> getElementsByTagName(const std::string& tagName);
    Element* querySelector(const std::string& selector);
    std::vector<Element*> querySelectorAll(const std::string& selector);
    
    // Events
    void addEventListener(const std::string& type, Value callback);
    
private:
    Window* window_;
    Element* documentElement_ = nullptr;
    Element* body_ = nullptr;
    Element* head_ = nullptr;
    std::string title_;
    
    std::vector<std::unique_ptr<Element>> ownedElements_;
};

/**
 * @brief Document builtin functions
 */
class DocumentBuiltin {
public:
    static Value getElementById(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value querySelector(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value createElement(Runtime::Context* ctx, const std::vector<Value>& args);
};

} // namespace Zepra::Browser
