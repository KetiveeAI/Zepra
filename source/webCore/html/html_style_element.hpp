/**
 * @file html_style_element.hpp
 * @brief HTMLStyleElement - Inline stylesheet element
 *
 * Implements the <style> element per HTML Living Standard.
 * Used for embedding CSS directly in HTML documents.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-style-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

// Forward declarations
class CSSStyleSheet;

/**
 * @brief HTMLStyleElement - inline stylesheets
 *
 * The <style> element allows authors to embed CSS style sheets
 * directly in their documents.
 */
class HTMLStyleElement : public HTMLElement {
public:
    HTMLStyleElement();
    ~HTMLStyleElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Media query for when styles apply
    std::string media() const;
    void setMedia(const std::string& media);

    /// MIME type (defaults to text/css)
    std::string type() const;
    void setType(const std::string& type);

    /// Blocking behavior
    std::string blocking() const;
    void setBlocking(const std::string& value);

    /// Whether styles are disabled
    bool disabled() const;
    void setDisabled(bool disabled);

    // =========================================================================
    // Stylesheet Access
    // =========================================================================

    /// Associated stylesheet object
    CSSStyleSheet* sheet() const;

    /// Get the CSS text content
    std::string cssText() const;

    /// Set the CSS text content
    void setCssText(const std::string& css);

    // =========================================================================
    // State
    // =========================================================================

    /// Whether stylesheet is parsed and ready
    bool isReady() const;

    /// Whether stylesheet applies to current document
    bool isApplicable() const;

    /// Check if media query matches
    bool mediaMatches() const;

    // =========================================================================
    // Parsing
    // =========================================================================

    /// Force re-parse of CSS content
    void reparse();

    /// Get any parse errors
    std::vector<std::string> parseErrors() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
