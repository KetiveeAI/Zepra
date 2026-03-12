/**
 * @file html_span_element.hpp
 * @brief HTMLSpanElement interface for <span> elements
 *
 * Simple inline container element.
 *
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-span-element
 */

#pragma once

#include "html/html_element.hpp"

namespace Zepra::WebCore {

/**
 * @brief HTMLSpanElement represents a <span> inline container element
 *
 * The span element has no special meaning - it's a generic inline container.
 * All functionality comes from HTMLElement base class.
 */
class HTMLSpanElement : public HTMLElement {
public:
    HTMLSpanElement();
    ~HTMLSpanElement() override;

    /// Clone node
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
};

} // namespace Zepra::WebCore
