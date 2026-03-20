// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file dom_bridge.cpp  
 * @brief DOM bridge implementation
 */

#include "integration/dom_bridge.hpp"
#include "browser/dom.hpp"
#include "rendering/page_renderer.hpp"
#include <memory>

namespace Zepra::Integration {

// =============================================================================
// DOMElementWrapper
// =============================================================================

DOMElementWrapper::DOMElementWrapper(WebCore::DOMElement* element)
    : element_(element) {}

DOMElementWrapper::~DOMElementWrapper() = default;

std::string DOMElementWrapper::getTagName() const {
    return element_ ? element_->tagName() : "";
}

std::string DOMElementWrapper::getId() const {
    return element_ ? element_->id() : "";
}

void DOMElementWrapper::setId(const std::string& id) {
    if (element_) element_->setId(id);
}

std::string DOMElementWrapper::getClassName() const {
    if (!element_) return "";
    auto classes = element_->classList();
    std::string result;
    for (const auto& c : classes) {
        if (!result.empty()) result += " ";
        result += c;
    }
    return result;
}

void DOMElementWrapper::setClassName(const std::string& cls) {
    if (element_) element_->setAttribute("class", cls);
}

std::string DOMElementWrapper::getInnerHTML() const {
    return element_ ? element_->innerHTML() : "";
}

void DOMElementWrapper::setInnerHTML(const std::string& html) {
    if (element_) element_->setInnerHTML(html);
}

std::string DOMElementWrapper::getTextContent() const {
    return element_ ? element_->textContent() : "";
}

void DOMElementWrapper::setTextContent(const std::string& text) {
    if (element_) element_->setTextContent(text);
}

std::string DOMElementWrapper::getAttribute(const std::string& name) const {
    return element_ ? element_->getAttribute(name) : "";
}

void DOMElementWrapper::setAttribute(const std::string& name, const std::string& value) {
    if (element_) element_->setAttribute(name, value);
}

bool DOMElementWrapper::hasAttribute(const std::string& name) const {
    return element_ ? element_->hasAttribute(name) : false;
}

void DOMElementWrapper::removeAttribute(const std::string& name) {
    if (element_) element_->removeAttribute(name);
}

DOMElementWrapper* DOMElementWrapper::parentElement() const {
    return nullptr;  // Would need access to parent wrapper cache
}

std::vector<DOMElementWrapper*> DOMElementWrapper::children() const {
    return {};  // Would need access to wrapper cache
}

DOMElementWrapper* DOMElementWrapper::querySelector(const std::string& selector) {
    return nullptr;  // Would need access to wrapper cache
}

std::vector<DOMElementWrapper*> DOMElementWrapper::querySelectorAll(const std::string& selector) {
    return {};  // Would need access to wrapper cache
}

void DOMElementWrapper::addEventListener(const std::string& type, const std::string& handlerScript) {
    eventHandlers_[type] = handlerScript;
}

void DOMElementWrapper::removeEventListener(const std::string& type) {
    eventHandlers_.erase(type);
}

void DOMElementWrapper::dispatchEvent(const std::string& type) {
    // Event dispatch would execute the handler script
    auto it = eventHandlers_.find(type);
    if (it != eventHandlers_.end()) {
        // TODO: Execute script through engine
    }
}

// =============================================================================
// DOMDocumentWrapper
// =============================================================================

DOMDocumentWrapper::DOMDocumentWrapper(WebCore::DOMDocument* doc)
    : doc_(doc) {}

DOMDocumentWrapper::~DOMDocumentWrapper() = default;

DOMElementWrapper* DOMDocumentWrapper::getOrCreateWrapper(WebCore::DOMElement* element) {
    if (!element) return nullptr;
    
    auto it = wrapperCache_.find(element);
    if (it != wrapperCache_.end()) {
        return it->second.get();
    }
    
    auto wrapper = std::make_unique<DOMElementWrapper>(element);
    auto* ptr = wrapper.get();
    wrapperCache_[element] = std::move(wrapper);
    return ptr;
}

DOMElementWrapper* DOMDocumentWrapper::documentElement() {
    return doc_ ? getOrCreateWrapper(doc_->documentElement()) : nullptr;
}

DOMElementWrapper* DOMDocumentWrapper::body() {
    return doc_ ? getOrCreateWrapper(doc_->body()) : nullptr;
}

DOMElementWrapper* DOMDocumentWrapper::head() {
    return doc_ ? getOrCreateWrapper(doc_->head()) : nullptr;
}

std::string DOMDocumentWrapper::getTitle() const {
    return doc_ ? doc_->title() : "";
}

void DOMDocumentWrapper::setTitle(const std::string& title) {
    if (doc_) doc_->setTitle(title);
}

DOMElementWrapper* DOMDocumentWrapper::createElement(const std::string& tagName) {
    if (!doc_) return nullptr;
    auto element = doc_->createElement(tagName);
    if (!element) return nullptr;
    // Store the raw pointer before releasing ownership
    auto* rawPtr = element.get();
    // Note: The document takes ownership through its internal mechanism
    return getOrCreateWrapper(rawPtr);
}

DOMElementWrapper* DOMDocumentWrapper::getElementById(const std::string& id) {
    if (!doc_) return nullptr;
    auto* element = doc_->getElementById(id);
    return getOrCreateWrapper(element);
}

std::vector<DOMElementWrapper*> DOMDocumentWrapper::getElementsByClassName(const std::string& className) {
    std::vector<DOMElementWrapper*> result;
    if (!doc_) return result;
    
    auto elements = doc_->getElementsByClassName(className);
    for (auto* el : elements) {
        if (auto* wrapper = getOrCreateWrapper(el)) {
            result.push_back(wrapper);
        }
    }
    return result;
}

std::vector<DOMElementWrapper*> DOMDocumentWrapper::getElementsByTagName(const std::string& tagName) {
    std::vector<DOMElementWrapper*> result;
    if (!doc_) return result;
    
    auto elements = doc_->getElementsByTagName(tagName);
    for (auto* el : elements) {
        if (auto* wrapper = getOrCreateWrapper(el)) {
            result.push_back(wrapper);
        }
    }
    return result;
}

DOMElementWrapper* DOMDocumentWrapper::querySelector(const std::string& selector) {
    if (!doc_) return nullptr;
    auto* element = doc_->querySelector(selector);
    return getOrCreateWrapper(element);
}

std::vector<DOMElementWrapper*> DOMDocumentWrapper::querySelectorAll(const std::string& selector) {
    std::vector<DOMElementWrapper*> result;
    if (!doc_) return result;
    
    auto elements = doc_->querySelectorAll(selector);
    for (auto* el : elements) {
        if (auto* wrapper = getOrCreateWrapper(el)) {
            result.push_back(wrapper);
        }
    }
    return result;
}

// =============================================================================
// DOMBridge
// =============================================================================

DOMBridge::DOMBridge(ScriptEngine* engine, WebCore::PageRenderer* renderer)
    : engine_(engine), renderer_(renderer) {}

DOMBridge::~DOMBridge() = default;

void DOMBridge::initialize() {
    registerNativeFunctions();
}

void DOMBridge::registerNativeFunctions() {
    // Native function registration would happen here
    // For now, we can inject a JS shim
}

void DOMBridge::setDocument(WebCore::DOMDocument* doc) {
    documentWrapper_ = std::make_unique<DOMDocumentWrapper>(doc);
}

void DOMBridge::onDOMChange(WebCore::DOMNode* node) {
    for (auto& callback : mutationObservers_) {
        callback({node});
    }
    requestUpdate();
}

void DOMBridge::requestUpdate() {
    if (!updatePending_) {
        updatePending_ = true;
        if (renderer_) {
            renderer_->invalidateAll();
        }
    }
}

void DOMBridge::addMutationObserver(MutationCallback callback) {
    mutationObservers_.push_back(std::move(callback));
}

} // namespace Zepra::Integration
