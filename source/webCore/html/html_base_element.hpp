/**
 * @file html_base_element.hpp
 * @brief HTMLBaseElement - Document base URL element
 *
 * Implements the <base> element per HTML Living Standard.
 * Specifies the base URL for relative URLs in the document.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-base-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLBaseElement - document base URL
 *
 * The <base> element allows authors to specify the document base URL
 * and/or the default browsing context for hyperlinks.
 * There should be at most one base element per document.
 */
class HTMLBaseElement : public HTMLElement {
public:
    HTMLBaseElement();
    ~HTMLBaseElement() override;

    // =========================================================================
    // Core Attributes
    // =========================================================================

    /// Base URL for relative URLs
    std::string href() const;
    void setHref(const std::string& href);

    /// Default target browsing context
    std::string target() const;
    void setTarget(const std::string& target);

    // =========================================================================
    // Computed Values
    // =========================================================================

    /// Resolved absolute base URL
    std::string resolvedHref() const;

    /// Whether href is a valid URL
    bool isValidHref() const;

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
