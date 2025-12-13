/**
 * @file html_span_element.cpp
 * @brief HTMLSpanElement implementation
 */

#include "webcore/html/html_span_element.hpp"

namespace Zepra::WebCore {

HTMLSpanElement::HTMLSpanElement() : HTMLElement("span") {}

HTMLSpanElement::~HTMLSpanElement() = default;

std::unique_ptr<DOMNode> HTMLSpanElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLSpanElement>();
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
