/**
 * @file html_head_element.hpp
 * @brief HTMLHeadElement - Document head container
 *
 * Implements the <head> element per HTML Living Standard.
 * Contains metadata like title, scripts, stylesheets.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-head-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Zepra::WebCore {

// Forward declarations
class HTMLTitleElement;
class HTMLMetaElement;
class HTMLLinkElement;
class HTMLStyleElement;
class HTMLScriptElement;
class HTMLBaseElement;

/**
 * @brief HTMLHeadElement - document head container
 *
 * The <head> element represents a collection of metadata for the Document.
 */
class HTMLHeadElement : public HTMLElement {
public:
    HTMLHeadElement();
    ~HTMLHeadElement() override;

    // =========================================================================
    // Convenience Accessors
    // =========================================================================

    /// Get the document title element
    HTMLTitleElement* titleElement() const;

    /// Get the base element (if any)
    HTMLBaseElement* baseElement() const;

    /// Get all meta elements
    std::vector<HTMLMetaElement*> metaElements() const;

    /// Get all link elements
    std::vector<HTMLLinkElement*> linkElements() const;

    /// Get all style elements
    std::vector<HTMLStyleElement*> styleElements() const;

    /// Get all script elements
    std::vector<HTMLScriptElement*> scriptElements() const;

    // =========================================================================
    // Metadata Queries
    // =========================================================================

    /// Get meta by name (e.g., "viewport", "description")
    HTMLMetaElement* getMetaByName(const std::string& name) const;

    /// Get meta by http-equiv
    HTMLMetaElement* getMetaByEquiv(const std::string& equiv) const;

    /// Get all stylesheets (link[rel=stylesheet])
    std::vector<HTMLLinkElement*> stylesheets() const;

    /// Get charset from meta
    std::string charset() const;

    /// Get viewport settings
    std::string viewport() const;

    /// Get document title text
    std::string titleText() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
