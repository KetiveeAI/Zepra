/**
 * @file html_embed_element.hpp
 * @brief HTML Embed elements - embed, object, source, track, picture
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTML Embed Element (<embed>)
 */
class HTMLEmbedElement : public HTMLElement {
public:
    HTMLEmbedElement();
    ~HTMLEmbedElement() override = default;
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& s) { setAttribute("src", s); }
    
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    std::string tagName() const { return "EMBED"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML Object Element (<object>)
 */
class HTMLObjectElement : public HTMLElement {
public:
    HTMLObjectElement();
    ~HTMLObjectElement() override = default;
    
    std::string data() const { return getAttribute("data"); }
    void setData(const std::string& d) { setAttribute("data", d); }
    
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    std::string useMap() const { return getAttribute("usemap"); }
    void setUseMap(const std::string& u) { setAttribute("usemap", u); }
    
    HTMLElement* form() const;
    HTMLElement* contentDocument() const;
    
    bool checkValidity() const;
    std::string validationMessage() const;
    
    std::string tagName() const { return "OBJECT"; }
    bool isFormControl() const { return true; }
};

/**
 * @brief HTML Param Element (<param>)
 */
class HTMLParamElement : public HTMLElement {
public:
    HTMLParamElement();
    ~HTMLParamElement() override = default;
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::string value() const { return getAttribute("value"); }
    void setValue(const std::string& v) { setAttribute("value", v); }
    
    std::string tagName() const { return "PARAM"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML Source Element (<source>)
 */
class HTMLSourceElement : public HTMLElement {
public:
    HTMLSourceElement();
    ~HTMLSourceElement() override = default;
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& s) { setAttribute("src", s); }
    
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string srcset() const { return getAttribute("srcset"); }
    void setSrcset(const std::string& s) { setAttribute("srcset", s); }
    
    std::string sizes() const { return getAttribute("sizes"); }
    void setSizes(const std::string& s) { setAttribute("sizes", s); }
    
    std::string media() const { return getAttribute("media"); }
    void setMedia(const std::string& m) { setAttribute("media", m); }
    
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    std::string tagName() const { return "SOURCE"; }
    bool isVoidElement() const { return true; }
};

/**
 * @brief HTML Track Element (<track>)
 */
class HTMLTrackElement : public HTMLElement {
public:
    enum class ReadyState { None = 0, Loading = 1, Loaded = 2, Error = 3 };
    
    HTMLTrackElement();
    ~HTMLTrackElement() override = default;
    
    std::string kind() const { return getAttribute("kind"); }
    void setKind(const std::string& k) { setAttribute("kind", k); }
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& s) { setAttribute("src", s); }
    
    std::string srclang() const { return getAttribute("srclang"); }
    void setSrclang(const std::string& l) { setAttribute("srclang", l); }
    
    std::string label() const { return getAttribute("label"); }
    void setLabel(const std::string& l) { setAttribute("label", l); }
    
    bool deflt() const { return hasAttribute("default"); }
    void setDefault(bool d) { if (d) setAttribute("default", ""); else removeAttribute("default"); }
    
    ReadyState readyState() const { return readyState_; }
    
    std::string tagName() const { return "TRACK"; }
    bool isVoidElement() const { return true; }
    
private:
    ReadyState readyState_ = ReadyState::None;
};

/**
 * @brief HTML Picture Element (<picture>)
 */
class HTMLPictureElement : public HTMLElement {
public:
    HTMLPictureElement();
    ~HTMLPictureElement() override = default;
    
    std::string tagName() const { return "PICTURE"; }
};

/**
 * @brief HTML Map Element (<map>)
 */
class HTMLMapElement : public HTMLElement {
public:
    HTMLMapElement();
    ~HTMLMapElement() override = default;
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    std::vector<HTMLElement*> areas() const;
    
    std::string tagName() const { return "MAP"; }
};

/**
 * @brief HTML Area Element (<area>)
 */
class HTMLAreaElement : public HTMLElement {
public:
    HTMLAreaElement();
    ~HTMLAreaElement() override = default;
    
    std::string alt() const { return getAttribute("alt"); }
    void setAlt(const std::string& a) { setAttribute("alt", a); }
    
    std::string coords() const { return getAttribute("coords"); }
    void setCoords(const std::string& c) { setAttribute("coords", c); }
    
    std::string shape() const { return getAttribute("shape"); }
    void setShape(const std::string& s) { setAttribute("shape", s); }
    
    std::string href() const { return getAttribute("href"); }
    void setHref(const std::string& h) { setAttribute("href", h); }
    
    std::string target() const { return getAttribute("target"); }
    void setTarget(const std::string& t) { setAttribute("target", t); }
    
    std::string download() const { return getAttribute("download"); }
    void setDownload(const std::string& d) { setAttribute("download", d); }
    
    std::string rel() const { return getAttribute("rel"); }
    void setRel(const std::string& r) { setAttribute("rel", r); }
    
    std::string tagName() const { return "AREA"; }
    bool isVoidElement() const { return true; }
};

} // namespace Zepra::WebCore
