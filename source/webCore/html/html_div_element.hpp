/**
 * @file html_div_element.hpp
 * @brief HTMLDivElement interface for <div> elements
 *
 * Simple block container element.
 *
 * @see https://html.spec.whatwg.org/multipage/grouping-content.html#the-div-element
 */

#pragma once

#include "html/html_element.hpp"

namespace Zepra::WebCore {

/**
 * @brief HTMLDivElement represents a <div> container element
 *
 * The div element has no special meaning - it's a generic container.
 * All functionality comes from HTMLElement base class.
 */
class HTMLDivElement : public HTMLElement {
public:
    HTMLDivElement();
    ~HTMLDivElement() override;

    /// Clone node
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
};

} // namespace Zepra::WebCore
