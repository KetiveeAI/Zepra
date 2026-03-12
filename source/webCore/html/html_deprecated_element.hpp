/**
 * @file html_deprecated_element.hpp
 * @brief Deprecated HTML elements (still needed for legacy support)
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief HTML Font Element (<font>) - Deprecated
 */
class HTMLFontElement : public HTMLElement {
public:
    HTMLFontElement();
    ~HTMLFontElement() override = default;
    
    std::string color() const { return getAttribute("color"); }
    void setColor(const std::string& c) { setAttribute("color", c); }
    
    std::string face() const { return getAttribute("face"); }
    void setFace(const std::string& f) { setAttribute("face", f); }
    
    std::string size() const { return getAttribute("size"); }
    void setSize(const std::string& s) { setAttribute("size", s); }
    
    std::string tagName() const { return "FONT"; }
};

/**
 * @brief HTML Center Element (<center>) - Deprecated
 */
class HTMLCenterElement : public HTMLElement {
public:
    HTMLCenterElement();
    ~HTMLCenterElement() override = default;
    
    std::string tagName() const { return "CENTER"; }
};

/**
 * @brief HTML Basefont Element (<basefont>) - Deprecated
 */
class HTMLBaseFontElement : public HTMLElement {
public:
    HTMLBaseFontElement();
    ~HTMLBaseFontElement() override = default;
    
    std::string color() const { return getAttribute("color"); }
    void setColor(const std::string& c) { setAttribute("color", c); }
    
    std::string face() const { return getAttribute("face"); }
    void setFace(const std::string& f) { setAttribute("face", f); }
    
    std::string size() const { return getAttribute("size"); }
    void setSize(const std::string& s) { setAttribute("size", s); }
    
    std::string tagName() const { return "BASEFONT"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML Strike Element (<strike>) - Deprecated
 */
class HTMLStrikeElement : public HTMLElement {
public:
    HTMLStrikeElement();
    ~HTMLStrikeElement() override = default;
    
    std::string tagName() const { return "STRIKE"; }
};

/**
 * @brief HTML TT Element (<tt>) - Deprecated
 */
class HTMLTTElement : public HTMLElement {
public:
    HTMLTTElement();
    ~HTMLTTElement() override = default;
    
    std::string tagName() const { return "TT"; }
};

/**
 * @brief HTML Big Element (<big>) - Deprecated
 */
class HTMLBigElement : public HTMLElement {
public:
    HTMLBigElement();
    ~HTMLBigElement() override = default;
    
    std::string tagName() const { return "BIG"; }
};

/**
 * @brief HTML Acronym Element (<acronym>) - Deprecated
 */
class HTMLAcronymElement : public HTMLElement {
public:
    HTMLAcronymElement();
    ~HTMLAcronymElement() override = default;
    
    std::string tagName() const { return "ACRONYM"; }
};

/**
 * @brief HTML Dir Element (<dir>) - Deprecated
 */
class HTMLDirElement : public HTMLElement {
public:
    HTMLDirElement();
    ~HTMLDirElement() override = default;
    
    bool compact() const { return hasAttribute("compact"); }
    void setCompact(bool c) { if (c) setAttribute("compact", ""); else removeAttribute("compact"); }
    
    std::string tagName() const { return "DIR"; }
};

/**
 * @brief HTML Applet Element (<applet>) - Deprecated
 */
class HTMLAppletElement : public HTMLElement {
public:
    HTMLAppletElement();
    ~HTMLAppletElement() override = default;
    
    std::string code() const { return getAttribute("code"); }
    void setCode(const std::string& c) { setAttribute("code", c); }
    
    std::string archive() const { return getAttribute("archive"); }
    void setArchive(const std::string& a) { setAttribute("archive", a); }
    
    std::string codeBase() const { return getAttribute("codebase"); }
    void setCodeBase(const std::string& c) { setAttribute("codebase", c); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string tagName() const { return "APPLET"; }
};

/**
 * @brief HTML Isindex Element (<isindex>) - Deprecated
 */
class HTMLIsIndexElement : public HTMLElement {
public:
    HTMLIsIndexElement();
    ~HTMLIsIndexElement() override = default;
    
    std::string prompt() const { return getAttribute("prompt"); }
    void setPrompt(const std::string& p) { setAttribute("prompt", p); }
    
    std::string tagName() const { return "ISINDEX"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML XMP Element (<xmp>) - Deprecated
 */
class HTMLXMPElement : public HTMLElement {
public:
    HTMLXMPElement();
    ~HTMLXMPElement() override = default;
    
    std::string tagName() const { return "XMP"; }
};

/**
 * @brief HTML Listing Element (<listing>) - Deprecated
 */
class HTMLListingElement : public HTMLElement {
public:
    HTMLListingElement();
    ~HTMLListingElement() override = default;
    
    std::string tagName() const { return "LISTING"; }
};

/**
 * @brief HTML PlainText Element (<plaintext>) - Deprecated
 */
class HTMLPlainTextElement : public HTMLElement {
public:
    HTMLPlainTextElement();
    ~HTMLPlainTextElement() override = default;
    
    std::string tagName() const { return "PLAINTEXT"; }
};

/**
 * @brief HTML Blink Element (<blink>) - Deprecated
 */
class HTMLBlinkElement : public HTMLElement {
public:
    HTMLBlinkElement();
    ~HTMLBlinkElement() override = default;
    
    std::string tagName() const { return "BLINK"; }
};

/**
 * @brief HTML Spacer Element (<spacer>) - Deprecated
 */
class HTMLSpacerElement : public HTMLElement {
public:
    HTMLSpacerElement();
    ~HTMLSpacerElement() override = default;
    
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string size() const { return getAttribute("size"); }
    void setSize(const std::string& s) { setAttribute("size", s); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    std::string tagName() const { return "SPACER"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML nobr Element (<nobr>) - Deprecated
 */
class HTMLNoBRElement : public HTMLElement {
public:
    HTMLNoBRElement();
    ~HTMLNoBRElement() override = default;
    
    std::string tagName() const { return "NOBR"; }
};

} // namespace Zepra::WebCore
