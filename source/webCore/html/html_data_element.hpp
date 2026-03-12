/**
 * @file html_data_element.hpp
 * @brief HTML Data elements - data, time, wbr, ruby
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief HTML Data Element (<data>)
 */
class HTMLDataElement : public HTMLElement {
public:
    HTMLDataElement();
    ~HTMLDataElement() override = default;
    
    std::string value() const { return getAttribute("value"); }
    void setValue(const std::string& v) { setAttribute("value", v); }
    
    std::string tagName() const { return "DATA"; }
};

/**
 * @brief HTML WBR Element (<wbr>) - Word Break Opportunity
 */
class HTMLWBRElement : public HTMLElement {
public:
    HTMLWBRElement();
    ~HTMLWBRElement() override = default;
    
    std::string tagName() const { return "WBR"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML Ruby Element (<ruby>)
 */
class HTMLRubyElement : public HTMLElement {
public:
    HTMLRubyElement();
    ~HTMLRubyElement() override = default;
    
    std::string tagName() const { return "RUBY"; }
};

/**
 * @brief HTML RP Element (<rp>) - Ruby Fallback Parenthesis
 */
class HTMLRPElement : public HTMLElement {
public:
    HTMLRPElement();
    ~HTMLRPElement() override = default;
    
    std::string tagName() const { return "RP"; }
};

/**
 * @brief HTML RT Element (<rt>) - Ruby Text
 */
class HTMLRTElement : public HTMLElement {
public:
    HTMLRTElement();
    ~HTMLRTElement() override = default;
    
    std::string tagName() const { return "RT"; }
};

/**
 * @brief HTML RTC Element (<rtc>) - Ruby Text Container
 */
class HTMLRTCElement : public HTMLElement {
public:
    HTMLRTCElement();
    ~HTMLRTCElement() override = default;
    
    std::string tagName() const { return "RTC"; }
};

/**
 * @brief HTML RB Element (<rb>) - Ruby Base
 */
class HTMLRBElement : public HTMLElement {
public:
    HTMLRBElement();
    ~HTMLRBElement() override = default;
    
    std::string tagName() const { return "RB"; }
};

/**
 * @brief HTML BDO Element (<bdo>) - Bidirectional Override
 */
class HTMLBDOElement : public HTMLElement {
public:
    HTMLBDOElement();
    ~HTMLBDOElement() override = default;
    
    std::string dir() const { return getAttribute("dir"); }
    void setDir(const std::string& d) { setAttribute("dir", d); }
    
    std::string tagName() const { return "BDO"; }
};

/**
 * @brief HTML BDI Element (<bdi>) - Bidirectional Isolate
 */
class HTMLBDIElement : public HTMLElement {
public:
    HTMLBDIElement();
    ~HTMLBDIElement() override = default;
    
    std::string tagName() const { return "BDI"; }
};

/**
 * @brief HTML Abbr Element (<abbr>)
 */
class HTMLAbbrElement : public HTMLElement {
public:
    HTMLAbbrElement();
    ~HTMLAbbrElement() override = default;
    
    std::string tagName() const { return "ABBR"; }
};

/**
 * @brief HTML Cite Element (<cite>)
 */
class HTMLCiteElement : public HTMLElement {
public:
    HTMLCiteElement();
    ~HTMLCiteElement() override = default;
    
    std::string tagName() const { return "CITE"; }
};

/**
 * @brief HTML DFN Element (<dfn>) - Definition
 */
class HTMLDFNElement : public HTMLElement {
public:
    HTMLDFNElement();
    ~HTMLDFNElement() override = default;
    
    std::string tagName() const { return "DFN"; }
};

/**
 * @brief HTML KBD Element (<kbd>) - Keyboard Input
 */
class HTMLKBDElement : public HTMLElement {
public:
    HTMLKBDElement();
    ~HTMLKBDElement() override = default;
    
    std::string tagName() const { return "KBD"; }
};

/**
 * @brief HTML SAMP Element (<samp>) - Sample Output
 */
class HTMLSAMPElement : public HTMLElement {
public:
    HTMLSAMPElement();
    ~HTMLSAMPElement() override = default;
    
    std::string tagName() const { return "SAMP"; }
};

/**
 * @brief HTML VAR Element (<var>) - Variable
 */
class HTMLVARElement : public HTMLElement {
public:
    HTMLVARElement();
    ~HTMLVARElement() override = default;
    
    std::string tagName() const { return "VAR"; }
};

/**
 * @brief HTML I Element (<i>) - Italic/Alternative Voice
 */
class HTMLIElement : public HTMLElement {
public:
    HTMLIElement();
    ~HTMLIElement() override = default;
    
    std::string tagName() const { return "I"; }
};

/**
 * @brief HTML B Element (<b>) - Bold/Bring Attention
 */
class HTMLBElement : public HTMLElement {
public:
    HTMLBElement();
    ~HTMLBElement() override = default;
    
    std::string tagName() const { return "B"; }
};

/**
 * @brief HTML U Element (<u>) - Underline
 */
class HTMLUElement : public HTMLElement {
public:
    HTMLUElement();
    ~HTMLUElement() override = default;
    
    std::string tagName() const { return "U"; }
};

/**
 * @brief HTML S Element (<s>) - Strikethrough
 */
class HTMLSElement : public HTMLElement {
public:
    HTMLSElement();
    ~HTMLSElement() override = default;
    
    std::string tagName() const { return "S"; }
};

/**
 * @brief HTML DEL Element (<del>) - Deleted Text
 */
class HTMLDelElement : public HTMLElement {
public:
    HTMLDelElement();
    ~HTMLDelElement() override = default;
    
    std::string cite() const { return getAttribute("cite"); }
    void setCite(const std::string& c) { setAttribute("cite", c); }
    
    std::string dateTime() const { return getAttribute("datetime"); }
    void setDateTime(const std::string& d) { setAttribute("datetime", d); }
    
    std::string tagName() const { return "DEL"; }
};

/**
 * @brief HTML INS Element (<ins>) - Inserted Text
 */
class HTMLInsElement : public HTMLElement {
public:
    HTMLInsElement();
    ~HTMLInsElement() override = default;
    
    std::string cite() const { return getAttribute("cite"); }
    void setCite(const std::string& c) { setAttribute("cite", c); }
    
    std::string dateTime() const { return getAttribute("datetime"); }
    void setDateTime(const std::string& d) { setAttribute("datetime", d); }
    
    std::string tagName() const { return "INS"; }
};

} // namespace Zepra::WebCore
