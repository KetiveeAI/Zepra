/**
 * @file html_unknown_element.hpp
 * @brief HTML Unknown Element for unrecognized tags
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief HTML Unknown Element - Fallback for unrecognized tags
 */
class HTMLUnknownElement : public HTMLElement {
public:
    explicit HTMLUnknownElement(const std::string& tagName);
    ~HTMLUnknownElement() override = default;
    
    std::string tagName() const { return tagName_; }
    
    bool isUnknown() const override { return true; }
    
private:
    std::string tagName_;
};

} // namespace Zepra::WebCore
