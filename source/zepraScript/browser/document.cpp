/**
 * @file document.cpp
 * @brief JavaScript Document and Element implementation
 */

#include "browser/document.hpp"
#include "browser/window.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Browser {

// =============================================================================
// Element Implementation
// =============================================================================

Element::Element(const std::string& tagName)
    : Object(Runtime::ObjectType::Ordinary)
    , tagName_(tagName) {}

std::string Element::getAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    return it != attributes_.end() ? it->second : "";
}

void Element::setAttribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
    
    // Special handling for id and class
    if (name == "id") id_ = value;
    else if (name == "class") className_ = value;
}

void Element::removeAttribute(const std::string& name) {
    attributes_.erase(name);
    if (name == "id") id_.clear();
    else if (name == "class") className_.clear();
}

bool Element::hasAttribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

Element* Element::firstChild() const {
    return children_.empty() ? nullptr : children_.front();
}

Element* Element::lastChild() const {
    return children_.empty() ? nullptr : children_.back();
}

void Element::appendChild(Element* child) {
    if (!child) return;
    
    // Remove from old parent
    if (child->parentElement_) {
        child->parentElement_->removeChild(child);
    }
    
    // Update sibling links
    if (!children_.empty()) {
        children_.back()->nextSibling_ = child;
        child->previousSibling_ = children_.back();
    }
    
    children_.push_back(child);
    child->parentElement_ = this;
    child->nextSibling_ = nullptr;
}

void Element::removeChild(Element* child) {
    if (!child || child->parentElement_ != this) return;
    
    // Update sibling links
    if (child->previousSibling_) {
        child->previousSibling_->nextSibling_ = child->nextSibling_;
    }
    if (child->nextSibling_) {
        child->nextSibling_->previousSibling_ = child->previousSibling_;
    }
    
    // Remove from children
    children_.erase(
        std::remove(children_.begin(), children_.end(), child),
        children_.end());
    
    child->parentElement_ = nullptr;
    child->previousSibling_ = nullptr;
    child->nextSibling_ = nullptr;
}

void Element::insertBefore(Element* newChild, Element* refChild) {
    if (!newChild) return;
    if (!refChild) {
        appendChild(newChild);
        return;
    }
    
    // Remove from old parent
    if (newChild->parentElement_) {
        newChild->parentElement_->removeChild(newChild);
    }
    
    // Find position
    auto it = std::find(children_.begin(), children_.end(), refChild);
    if (it == children_.end()) {
        appendChild(newChild);
        return;
    }
    
    // Update sibling links
    newChild->nextSibling_ = refChild;
    newChild->previousSibling_ = refChild->previousSibling_;
    if (refChild->previousSibling_) {
        refChild->previousSibling_->nextSibling_ = newChild;
    }
    refChild->previousSibling_ = newChild;
    
    children_.insert(it, newChild);
    newChild->parentElement_ = this;
}

Element* Element::cloneNode(bool deep) const {
    Element* clone = new Element(tagName_);
    clone->id_ = id_;
    clone->className_ = className_;
    clone->innerHTML_ = innerHTML_;
    clone->textContent_ = textContent_;
    clone->attributes_ = attributes_;
    
    if (deep) {
        for (Element* child : children_) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

Element* Element::querySelector(const std::string& selector) const {
    // Simple selector support: #id, .class, tagname
    if (selector.empty()) return nullptr;
    
    if (selector[0] == '#') {
        return getElementById(selector.substr(1));
    } else if (selector[0] == '.') {
        auto results = getElementsByClassName(selector.substr(1));
        return results.empty() ? nullptr : results[0];
    } else {
        auto results = getElementsByTagName(selector);
        return results.empty() ? nullptr : results[0];
    }
}

std::vector<Element*> Element::querySelectorAll(const std::string& selector) const {
    if (selector.empty()) return {};
    
    if (selector[0] == '#') {
        Element* el = getElementById(selector.substr(1));
        return el ? std::vector<Element*>{el} : std::vector<Element*>{};
    } else if (selector[0] == '.') {
        return getElementsByClassName(selector.substr(1));
    } else {
        return getElementsByTagName(selector);
    }
}

Element* Element::getElementById(const std::string& id) const {
    for (Element* child : children_) {
        if (child->id_ == id) return child;
        Element* found = child->getElementById(id);
        if (found) return found;
    }
    return nullptr;
}

std::vector<Element*> Element::getElementsByClassName(const std::string& className) const {
    std::vector<Element*> result;
    for (Element* child : children_) {
        if (child->className_.find(className) != std::string::npos) {
            result.push_back(child);
        }
        auto childResults = child->getElementsByClassName(className);
        result.insert(result.end(), childResults.begin(), childResults.end());
    }
    return result;
}

std::vector<Element*> Element::getElementsByTagName(const std::string& tagName) const {
    std::vector<Element*> result;
    for (Element* child : children_) {
        if (child->tagName_ == tagName) {
            result.push_back(child);
        }
        auto childResults = child->getElementsByTagName(tagName);
        result.insert(result.end(), childResults.begin(), childResults.end());
    }
    return result;
}

void Element::addEventListener(const std::string& type, Value callback) {
    eventListeners_[type].push_back(callback);
}

void Element::removeEventListener(const std::string& type, Value) {
    // TODO: Match and remove specific callback
    eventListeners_[type].clear();
}

void Element::dispatchEvent(const std::string& type, Value eventData) {
    auto it = eventListeners_.find(type);
    if (it != eventListeners_.end()) {
        for (const auto& callback : it->second) {
            // Call callback through VM using Function::call()
            if (callback.isObject()) {
                Runtime::Object* obj = callback.asObject();
                if (obj && obj->isFunction()) {
                    Runtime::Function* fn = static_cast<Runtime::Function*>(obj);
                    // Create event argument vector
                    std::vector<Runtime::Value> args;
                    if (!eventData.isUndefined()) {
                        args.push_back(eventData);
                    }
                    // Invoke the callback with 'this' as the element
                    // Note: ctx is nullptr here - for full support, Element should store context
                    fn->call(nullptr, Runtime::Value::object(this), args);
                }
            }
        }
    }
}

// =============================================================================
// Document Implementation
// =============================================================================

Document::Document(Window* window)
    : Object(Runtime::ObjectType::Ordinary)
    , window_(window) {
    
    // Create basic DOM structure
    documentElement_ = createElement("html");
    head_ = createElement("head");
    body_ = createElement("body");
    
    documentElement_->appendChild(head_);
    documentElement_->appendChild(body_);
}

Element* Document::createElement(const std::string& tagName) {
    auto element = std::make_unique<Element>(tagName);
    Element* ptr = element.get();
    ownedElements_.push_back(std::move(element));
    return ptr;
}

Element* Document::createTextNode(const std::string& text) {
    Element* node = createElement("#text");
    node->setTextContent(text);
    return node;
}

Element* Document::getElementById(const std::string& id) {
    return documentElement_ ? documentElement_->getElementById(id) : nullptr;
}

std::vector<Element*> Document::getElementsByClassName(const std::string& className) {
    return documentElement_ ? documentElement_->getElementsByClassName(className) 
                            : std::vector<Element*>{};
}

std::vector<Element*> Document::getElementsByTagName(const std::string& tagName) {
    return documentElement_ ? documentElement_->getElementsByTagName(tagName)
                            : std::vector<Element*>{};
}

Element* Document::querySelector(const std::string& selector) {
    return documentElement_ ? documentElement_->querySelector(selector) : nullptr;
}

std::vector<Element*> Document::querySelectorAll(const std::string& selector) {
    return documentElement_ ? documentElement_->querySelectorAll(selector)
                            : std::vector<Element*>{};
}

void Document::addEventListener(const std::string& type, Value callback) {
    if (documentElement_) {
        documentElement_->addEventListener(type, callback);
    }
}

// =============================================================================
// DocumentBuiltin Implementation
// =============================================================================

Value DocumentBuiltin::getElementById(Runtime::Context*, const std::vector<Value>&) {
    // TODO: Get document from context and call getElementById
    return Value::null();
}

Value DocumentBuiltin::querySelector(Runtime::Context*, const std::vector<Value>&) {
    return Value::null();
}

Value DocumentBuiltin::createElement(Runtime::Context*, const std::vector<Value>&) {
    return Value::null();
}

} // namespace Zepra::Browser
