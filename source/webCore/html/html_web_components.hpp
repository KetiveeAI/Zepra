/**
 * @file html_web_components.hpp
 * @brief Web Components support (combined utilities)
 */

#pragma once

#include "html/html_element.hpp"
#include "html/html_custom_element.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTML Slot Element utilities
 */
class SlotUtils {
public:
    /**
     * @brief Get assigned nodes for a slot
     */
    static std::vector<HTMLElement*> getAssignedNodes(HTMLElement* slot, bool flatten = false);
    
    /**
     * @brief Get assigned slot for an element
     */
    static HTMLElement* getAssignedSlot(HTMLElement* element);
    
    /**
     * @brief Assign element to slot
     */
    static void assignNodeToSlot(HTMLElement* node, HTMLElement* slot);
};

/**
 * @brief Shadow DOM utilities
 */
class ShadowDOMUtils {
public:
    /**
     * @brief Attach shadow root to element
     */
    static ShadowRoot* attachShadow(HTMLElement* host, ShadowRoot::Mode mode);
    
    /**
     * @brief Get shadow root
     */
    static ShadowRoot* getShadowRoot(HTMLElement* host);
    
    /**
     * @brief Check if element is in shadow tree
     */
    static bool isInShadowTree(HTMLElement* element);
    
    /**
     * @brief Get shadow host
     */
    static HTMLElement* getShadowHost(HTMLElement* element);
    
    /**
     * @brief Get composed path
     */
    static std::vector<HTMLElement*> composedPath(HTMLElement* target);
};

/**
 * @brief Adopted stylesheets for shadow DOM
 */
class CSSStyleSheet {
public:
    CSSStyleSheet() = default;
    ~CSSStyleSheet() = default;
    
    void replaceSync(const std::string& css);
    void replace(const std::string& css, std::function<void()> callback);
    
    void insertRule(const std::string& rule, size_t index = 0);
    void deleteRule(size_t index);
    
    std::string cssText() const { return cssText_; }
    
private:
    std::string cssText_;
    std::vector<std::string> rules_;
};

/**
 * @brief Document fragment
 */
class DocumentFragment : public HTMLElement {
public:
    DocumentFragment();
    ~DocumentFragment() override = default;
    
    std::string tagName() const { return "#document-fragment"; }
    
    HTMLElement* querySelector(const std::string& selector) const;
    std::vector<HTMLElement*> querySelectorAll(const std::string& selector) const;
};

/**
 * @brief HTML imports (deprecated but still used)
 */
class HTMLImport {
public:
    HTMLImport(const std::string& href);
    ~HTMLImport() = default;
    
    std::string href() const { return href_; }
    bool loaded() const { return loaded_; }
    
    HTMLElement* import() const { return import_; }
    
    std::function<void()> onLoad;
    std::function<void()> onError;
    
private:
    std::string href_;
    bool loaded_ = false;
    HTMLElement* import_ = nullptr;
};

} // namespace Zepra::WebCore
