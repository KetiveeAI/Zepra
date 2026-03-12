/**
 * @file html_list_element.hpp
 * @brief HTML List elements - ul, ol, li, dl, dt, dd
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief HTML Unordered List Element (<ul>)
 */
class HTMLUListElement : public HTMLElement {
public:
    HTMLUListElement();
    ~HTMLUListElement() override = default;
    
    std::string tagName() const { return "UL"; }
};

/**
 * @brief HTML Ordered List Element (<ol>)
 */
class HTMLOListElement : public HTMLElement {
public:
    HTMLOListElement();
    ~HTMLOListElement() override = default;
    
    // Start number
    int start() const { return start_; }
    void setStart(int s) { start_ = s; }
    
    // Reversed
    bool reversed() const { return hasAttribute("reversed"); }
    void setReversed(bool r) { if (r) setAttribute("reversed", ""); else removeAttribute("reversed"); }
    
    // Type (1, a, A, i, I)
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string tagName() const { return "OL"; }
    
private:
    int start_ = 1;
};

/**
 * @brief HTML List Item Element (<li>)
 */
class HTMLLIElement : public HTMLElement {
public:
    HTMLLIElement();
    ~HTMLLIElement() override = default;
    
    // Value (number in ordered list)
    int value() const { return value_; }
    void setValue(int v) { value_ = v; }
    
    std::string tagName() const { return "LI"; }
    
private:
    int value_ = -1;  // -1 = auto
};

/**
 * @brief HTML Description List Element (<dl>)
 */
class HTMLDListElement : public HTMLElement {
public:
    HTMLDListElement();
    ~HTMLDListElement() override = default;
    
    std::string tagName() const { return "DL"; }
};

/**
 * @brief HTML Description Term Element (<dt>)
 */
class HTMLDTElement : public HTMLElement {
public:
    HTMLDTElement();
    ~HTMLDTElement() override = default;
    
    std::string tagName() const { return "DT"; }
};

/**
 * @brief HTML Description Details Element (<dd>)
 */
class HTMLDDElement : public HTMLElement {
public:
    HTMLDDElement();
    ~HTMLDDElement() override = default;
    
    std::string tagName() const { return "DD"; }
};

/**
 * @brief HTML Menu Element (<menu>)
 */
class HTMLMenuElement : public HTMLElement {
public:
    HTMLMenuElement();
    ~HTMLMenuElement() override = default;
    
    std::string tagName() const { return "MENU"; }
};

} // namespace Zepra::WebCore
