/**
 * @file html_quote_element.hpp
 * @brief HTMLQuoteElement - Quotation elements
 *
 * Implements <blockquote> and <q> elements per HTML Living Standard.
 * Block and inline quotations.
 *
 * @see https://html.spec.whatwg.org/multipage/grouping-content.html#the-blockquote-element
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-q-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLQuoteElement - base for quotation elements
 *
 * Base class for both <blockquote> and <q> elements.
 */
class HTMLQuoteElement : public HTMLElement {
public:
    explicit HTMLQuoteElement(const std::string& tagName);
    ~HTMLQuoteElement() override;

    // =========================================================================
    // Core Attribute
    // =========================================================================

    /// URL of the quotation source
    std::string cite() const;
    void setCite(const std::string& cite);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLBlockQuoteElement - block quotation
 *
 * The <blockquote> element represents a section that is quoted from another source.
 */
class HTMLBlockQuoteElement : public HTMLQuoteElement {
public:
    HTMLBlockQuoteElement();
    ~HTMLBlockQuoteElement() override = default;
};

/**
 * @brief HTMLQElement - inline quotation
 *
 * The <q> element represents an inline quotation.
 * Browsers typically add quotation marks automatically.
 */
class HTMLQElement : public HTMLQuoteElement {
public:
    HTMLQElement();
    ~HTMLQElement() override = default;
};

/**
 * @brief HTMLCiteElement - citation
 *
 * The <cite> element represents the title of a work.
 */
class HTMLCiteElement : public HTMLElement {
public:
    HTMLCiteElement();
    ~HTMLCiteElement() override;

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
