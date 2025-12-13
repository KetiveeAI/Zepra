/**
 * @file html_div_element.cpp
 * @brief HTMLDivElement implementation
 */

#include "webcore/html/html_div_element.hpp"

namespace Zepra::WebCore {

HTMLDivElement::HTMLDivElement() : HTMLElement("div") {}

HTMLDivElement::~HTMLDivElement() = default;

std::unique_ptr<DOMNode> HTMLDivElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLDivElement>();
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
