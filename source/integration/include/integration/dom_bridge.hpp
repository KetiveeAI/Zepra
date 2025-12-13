/**
 * @file dom_bridge.hpp
 * @brief Bridges ZebraScript DOM to webCore DOM
 * 
 * Enables JavaScript DOM manipulation to affect the rendered page.
 * Uses opaque handles to avoid template instantiation issues.
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>

// Forward declarations only - no template containers with incomplete types
namespace Zepra {
    class ScriptEngine;
}

namespace Zepra::WebCore {
    class DOMDocument;
    class DOMElement;
    class DOMNode;
    class PageRenderer;
}

namespace Zepra::Integration {

/**
 * @brief Opaque handle for JavaScript values
 */
using JSValueHandle = void*;

/**
 * @brief Wraps a webCore DOM element for JavaScript access
 */
class DOMElementWrapper {
public:
    explicit DOMElementWrapper(WebCore::DOMElement* element);
    ~DOMElementWrapper();
    
    WebCore::DOMElement* wrapped() const { return element_; }
    
    // JS property accessors
    std::string getTagName() const;
    std::string getId() const;
    void setId(const std::string& id);
    std::string getClassName() const;
    void setClassName(const std::string& cls);
    std::string getInnerHTML() const;
    void setInnerHTML(const std::string& html);
    std::string getTextContent() const;
    void setTextContent(const std::string& text);
    
    // Attributes
    std::string getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    bool hasAttribute(const std::string& name) const;
    void removeAttribute(const std::string& name);
    
    // Tree traversal
    DOMElementWrapper* parentElement() const;
    std::vector<DOMElementWrapper*> children() const;
    
    // Query
    DOMElementWrapper* querySelector(const std::string& selector);
    std::vector<DOMElementWrapper*> querySelectorAll(const std::string& selector);
    
    // Events - use string callbacks for simplicity
    void addEventListener(const std::string& type, const std::string& handlerScript);
    void removeEventListener(const std::string& type);
    void dispatchEvent(const std::string& type);
    
private:
    WebCore::DOMElement* element_;
    std::unordered_map<std::string, std::string> eventHandlers_;
};

/**
 * @brief Wraps webCore DOMDocument for JavaScript
 */
class DOMDocumentWrapper {
public:
    explicit DOMDocumentWrapper(WebCore::DOMDocument* doc);
    ~DOMDocumentWrapper();
    
    WebCore::DOMDocument* wrapped() const { return doc_; }
    
    // Document properties
    DOMElementWrapper* documentElement();
    DOMElementWrapper* body();
    DOMElementWrapper* head();
    std::string getTitle() const;
    void setTitle(const std::string& title);
    
    // Element creation
    DOMElementWrapper* createElement(const std::string& tagName);
    
    // Query methods
    DOMElementWrapper* getElementById(const std::string& id);
    std::vector<DOMElementWrapper*> getElementsByClassName(const std::string& className);
    std::vector<DOMElementWrapper*> getElementsByTagName(const std::string& tagName);
    DOMElementWrapper* querySelector(const std::string& selector);
    std::vector<DOMElementWrapper*> querySelectorAll(const std::string& selector);
    
private:
    WebCore::DOMDocument* doc_;
    std::unordered_map<WebCore::DOMElement*, std::unique_ptr<DOMElementWrapper>> wrapperCache_;
    
    DOMElementWrapper* getOrCreateWrapper(WebCore::DOMElement* element);
};

/**
 * @brief Main DOM bridge connecting JS engine to webCore
 */
class DOMBridge {
public:
    DOMBridge(ScriptEngine* engine, WebCore::PageRenderer* renderer);
    ~DOMBridge();
    
    /**
     * @brief Initialize the bridge (inject globals)
     */
    void initialize();
    
    /**
     * @brief Set the document for this bridge
     */
    void setDocument(WebCore::DOMDocument* doc);
    
    /**
     * @brief Get the document wrapper
     */
    DOMDocumentWrapper* document() const { return documentWrapper_.get(); }
    
    /**
     * @brief Handle DOM mutation
     */
    void onDOMChange(WebCore::DOMNode* node);
    
    /**
     * @brief Trigger layout/repaint after JS mutations
     */
    void requestUpdate();
    
    /**
     * @brief Get the script engine
     */
    ScriptEngine* engine() const { return engine_; }
    
    /**
     * @brief Get the page renderer
     */
    WebCore::PageRenderer* renderer() const { return renderer_; }
    
    // Mutation observer support
    using MutationCallback = std::function<void(const std::vector<WebCore::DOMNode*>&)>;
    void addMutationObserver(MutationCallback callback);
    
private:
    ScriptEngine* engine_;
    WebCore::PageRenderer* renderer_;
    std::unique_ptr<DOMDocumentWrapper> documentWrapper_;
    std::vector<MutationCallback> mutationObservers_;
    bool updatePending_ = false;
    
    void registerNativeFunctions();
};

} // namespace Zepra::Integration
