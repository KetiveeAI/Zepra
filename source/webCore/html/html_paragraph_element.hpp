/**
 * @file html_paragraph_element.hpp
 * @brief HTMLParagraphElement - Paragraph element
 *
 * Implements the <p> element per HTML Living Standard.
 * The most common text block element.
 *
 * @see https://html.spec.whatwg.org/multipage/grouping-content.html#the-p-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLParagraphElement - paragraph
 *
 * The <p> element represents a paragraph.
 */
class HTMLParagraphElement : public HTMLElement {
public:
    HTMLParagraphElement();
    ~HTMLParagraphElement() override;

    // =========================================================================
    // Deprecated Attributes (for compatibility)
    // =========================================================================

    /// Text alignment (deprecated, use CSS text-align)
    std::string align() const;
    void setAlign(const std::string& align);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
