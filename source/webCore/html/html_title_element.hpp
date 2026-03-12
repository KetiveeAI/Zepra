/**
 * @file html_title_element.hpp
 * @brief HTMLTitleElement - Document title element
 *
 * Implements the <title> element per HTML Living Standard.
 * Defines the document's title shown in browser tabs.
 *
 * @see https://html.spec.whatwg.org/multipage/semantics.html#the-title-element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLTitleElement - document title
 *
 * The <title> element represents the document's title or name.
 * There should be only one title element per document.
 */
class HTMLTitleElement : public HTMLElement {
public:
    HTMLTitleElement();
    ~HTMLTitleElement() override;

    // =========================================================================
    // Core Property
    // =========================================================================

    /// The title text content
    std::string text() const;
    void setText(const std::string& text);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
