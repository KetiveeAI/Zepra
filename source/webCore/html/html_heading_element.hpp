/**
 * @file html_heading_element.hpp
 * @brief HTML Heading Element (h1-h6)
 * 
 * Other element types are in their dedicated headers:
 * - Paragraph: html_paragraph_element.hpp
 * - Break elements (br, hr, wbr): html_break_element.hpp
 * - Preformatted (pre, code, kbd, samp, var): html_preformatted_element.hpp
 * - Quote (blockquote, q, cite): html_quote_element.hpp
 * - Text formatting (em, strong, small, mark, sub, sup, etc.): html_text_formatting.hpp
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <algorithm>

namespace Zepra::WebCore {

/**
 * @brief HTML Heading Element (<h1>-<h6>)
 */
class HTMLHeadingElement : public HTMLElement {
public:
    explicit HTMLHeadingElement(int level = 1);
    ~HTMLHeadingElement() override = default;
    
    int level() const { return level_; }
    void setLevel(int l) { level_ = std::clamp(l, 1, 6); }
    
    std::string tagName() const {
        return "H" + std::to_string(level_);
    }
    
private:
    int level_;
};

} // namespace Zepra::WebCore
