/**
 * @file dom.cpp
 * @brief DOM implementation
 */

#include "webcore/dom.hpp"
#include <algorithm>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// DOMNode
// =============================================================================

DOMNode::DOMNode(NodeType type) : nodeType_(type) {}
DOMNode::~DOMNode() = default;

DOMNode* DOMNode::previousSibling() const {
    if (!parent_) return nullptr;
    
    const auto& siblings = parent_->children_;
    for (size_t i = 1; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return siblings[i - 1].get();
        }
    }
    return nullptr;
}

DOMNode* DOMNode::nextSibling() const {
    if (!parent_) return nullptr;
    
    const auto& siblings = parent_->children_;
    for (size_t i = 0; i + 1 < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return siblings[i + 1].get();
        }
    }
    return nullptr;
}

DOMNode* DOMNode::appendChild(std::unique_ptr<DOMNode> child) {
    child->parent_ = this;
    child->document_ = document_;
    DOMNode* ptr = child.get();
    children_.push_back(std::move(child));
    return ptr;
}

DOMNode* DOMNode::insertBefore(std::unique_ptr<DOMNode> child, DOMNode* refChild) {
    if (!refChild) return appendChild(std::move(child));
    
    child->parent_ = this;
    child->document_ = document_;
    DOMNode* ptr = child.get();
    
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == refChild) {
            children_.insert(it, std::move(child));
            return ptr;
        }
    }
    
    return appendChild(std::move(child));
}

std::unique_ptr<DOMNode> DOMNode::removeChild(DOMNode* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            std::unique_ptr<DOMNode> removed = std::move(*it);
            removed->parent_ = nullptr;
            children_.erase(it);
            return removed;
        }
    }
    return nullptr;
}

DOMNode* DOMNode::replaceChild(std::unique_ptr<DOMNode> newChild, DOMNode* oldChild) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == oldChild) {
            newChild->parent_ = this;
            newChild->document_ = document_;
            DOMNode* ptr = newChild.get();
            (*it)->parent_ = nullptr;
            *it = std::move(newChild);
            return ptr;
        }
    }
    return nullptr;
}

bool DOMNode::contains(DOMNode* other) const {
    if (!other) return false;
    
    DOMNode* node = other;
    while (node) {
        if (node == this) return true;
        node = node->parent_;
    }
    return false;
}

// =============================================================================
// DOMText
// =============================================================================

DOMText::DOMText(const std::string& data) 
    : DOMNode(NodeType::Text), data_(data) {}

std::unique_ptr<DOMNode> DOMText::cloneNode(bool) const {
    return std::make_unique<DOMText>(data_);
}

// =============================================================================
// DOMComment
// =============================================================================

DOMComment::DOMComment(const std::string& data) 
    : DOMNode(NodeType::Comment), data_(data) {}

std::unique_ptr<DOMNode> DOMComment::cloneNode(bool) const {
    return std::make_unique<DOMComment>(data_);
}

// =============================================================================
// DOMElement
// =============================================================================

DOMElement::DOMElement(const std::string& tagName) 
    : DOMNode(NodeType::Element), tagName_(tagName) {}

std::string DOMElement::getAttribute(const std::string& name) const {
    auto it = attributes_.find(name);
    return it != attributes_.end() ? it->second : "";
}

void DOMElement::setAttribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
}

void DOMElement::removeAttribute(const std::string& name) {
    attributes_.erase(name);
}

bool DOMElement::hasAttribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

std::vector<std::string> DOMElement::classList() const {
    std::vector<std::string> classes;
    std::istringstream iss(className());
    std::string cls;
    while (iss >> cls) {
        classes.push_back(cls);
    }
    return classes;
}

std::string DOMElement::textContent() const {
    std::string content;
    for (const auto& child : children_) {
        if (child->nodeType() == NodeType::Text) {
            content += static_cast<DOMText*>(child.get())->data();
        } else if (child->nodeType() == NodeType::Element) {
            content += static_cast<DOMElement*>(child.get())->textContent();
        }
    }
    return content;
}

void DOMElement::setTextContent(const std::string& text) {
    children_.clear();
    appendChild(std::make_unique<DOMText>(text));
}

DOMElement* DOMElement::getElementById(const std::string& id) {
    if (this->id() == id) return this;
    
    for (const auto& child : children_) {
        if (child->nodeType() == NodeType::Element) {
            if (DOMElement* found = static_cast<DOMElement*>(child.get())->getElementById(id)) {
                return found;
            }
        }
    }
    return nullptr;
}

std::vector<DOMElement*> DOMElement::getElementsByTagName(const std::string& tagName) {
    std::vector<DOMElement*> elements;
    
    if (tagName_ == tagName || tagName == "*") {
        elements.push_back(this);
    }
    
    for (const auto& child : children_) {
        if (child->nodeType() == NodeType::Element) {
            auto childElements = static_cast<DOMElement*>(child.get())->getElementsByTagName(tagName);
            elements.insert(elements.end(), childElements.begin(), childElements.end());
        }
    }
    
    return elements;
}

std::vector<DOMElement*> DOMElement::getElementsByClassName(const std::string& className) {
    std::vector<DOMElement*> elements;
    
    auto classes = classList();
    if (std::find(classes.begin(), classes.end(), className) != classes.end()) {
        elements.push_back(this);
    }
    
    for (const auto& child : children_) {
        if (child->nodeType() == NodeType::Element) {
            auto childElements = static_cast<DOMElement*>(child.get())->getElementsByClassName(className);
            elements.insert(elements.end(), childElements.begin(), childElements.end());
        }
    }
    
    return elements;
}

DOMElement* DOMElement::querySelector(const std::string&) {
    // TODO: Implement CSS selector matching
    return nullptr;
}

std::vector<DOMElement*> DOMElement::querySelectorAll(const std::string&) {
    // TODO: Implement CSS selector matching
    return {};
}

std::string DOMElement::style(const std::string& property) const {
    auto it = styleMap_.find(property);
    return it != styleMap_.end() ? it->second : "";
}

void DOMElement::setStyle(const std::string& property, const std::string& value) {
    styleMap_[property] = value;
}



std::unique_ptr<DOMNode> DOMElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<DOMElement>(tagName_);
    clone->attributes_ = attributes_;
    clone->styleMap_ = styleMap_;
    
    if (deep) {
        for (const auto& child : children_) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

// =============================================================================
// DOMDocument
// =============================================================================

DOMDocument::DOMDocument() : DOMNode(NodeType::Document) {
    document_ = this;
}

DOMElement* DOMDocument::body() const {
    if (!documentElement_) return nullptr;
    for (const auto& child : documentElement_->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto elem = static_cast<DOMElement*>(child.get());
            if (elem->tagName() == "body" || elem->tagName() == "BODY") {
                return elem;
            }
        }
    }
    return nullptr;
}

DOMElement* DOMDocument::head() const {
    if (!documentElement_) return nullptr;
    for (const auto& child : documentElement_->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto elem = static_cast<DOMElement*>(child.get());
            if (elem->tagName() == "head" || elem->tagName() == "HEAD") {
                return elem;
            }
        }
    }
    return nullptr;
}

std::string DOMDocument::title() const {
    DOMElement* h = head();
    if (!h) return "";
    
    for (const auto& child : h->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto elem = static_cast<DOMElement*>(child.get());
            if (elem->tagName() == "title" || elem->tagName() == "TITLE") {
                return elem->textContent();
            }
        }
    }
    return "";
}

void DOMDocument::setTitle(const std::string& title) {
    DOMElement* h = head();
    if (!h) return;
    
    for (const auto& child : h->childNodes()) {
        if (child->nodeType() == NodeType::Element) {
            auto elem = static_cast<DOMElement*>(child.get());
            if (elem->tagName() == "title" || elem->tagName() == "TITLE") {
                elem->setTextContent(title);
                return;
            }
        }
    }
    
    auto titleElem = createElement("title");
    titleElem->setTextContent(title);
    h->appendChild(std::move(titleElem));
}

std::unique_ptr<DOMElement> DOMDocument::createElement(const std::string& tagName) {
    auto elem = std::make_unique<DOMElement>(tagName);
    elem->setOwnerDocument(this);
    return elem;
}

std::unique_ptr<DOMText> DOMDocument::createTextNode(const std::string& data) {
    auto text = std::make_unique<DOMText>(data);
    text->setOwnerDocument(this);
    return text;
}

std::unique_ptr<DOMComment> DOMDocument::createComment(const std::string& data) {
    auto comment = std::make_unique<DOMComment>(data);
    comment->setOwnerDocument(this);
    return comment;
}

DOMElement* DOMDocument::getElementById(const std::string& id) {
    if (documentElement_) {
        return documentElement_->getElementById(id);
    }
    return nullptr;
}

std::vector<DOMElement*> DOMDocument::getElementsByTagName(const std::string& tagName) {
    if (documentElement_) {
        return documentElement_->getElementsByTagName(tagName);
    }
    return {};
}

std::vector<DOMElement*> DOMDocument::getElementsByClassName(const std::string& className) {
    if (documentElement_) {
        return documentElement_->getElementsByClassName(className);
    }
    return {};
}

DOMElement* DOMDocument::querySelector(const std::string& selector) {
    if (documentElement_) {
        return documentElement_->querySelector(selector);
    }
    return nullptr;
}

std::vector<DOMElement*> DOMDocument::querySelectorAll(const std::string& selector) {
    if (documentElement_) {
        return documentElement_->querySelectorAll(selector);
    }
    return {};
}

std::unique_ptr<DOMNode> DOMDocument::cloneNode(bool deep) const {
    auto clone = std::make_unique<DOMDocument>();
    
    if (deep && documentElement_) {
        auto elemClone = documentElement_->cloneNode(true);
        clone->documentElement_ = static_cast<DOMElement*>(clone->appendChild(std::move(elemClone)));
    }
    
    return clone;
}



} // namespace Zepra::WebCore
