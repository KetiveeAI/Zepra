/**
 * @file html_iframe_element.hpp
 * @brief HTML IFrame Element
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

class Document;

/**
 * @brief Sandbox flags for iframe
 */
enum class SandboxFlag : uint32_t {
    None                    = 0,
    AllowForms              = 1 << 0,
    AllowPointerLock        = 1 << 1,
    AllowPopups             = 1 << 2,
    AllowSameOrigin         = 1 << 3,
    AllowScripts            = 1 << 4,
    AllowTopNavigation      = 1 << 5,
    AllowPresentation       = 1 << 6,
    AllowModals             = 1 << 7,
    AllowOrientationLock    = 1 << 8,
    AllowPopupsToEscapeSandbox = 1 << 9,
    AllowDownloads          = 1 << 10,
};

inline SandboxFlag operator|(SandboxFlag a, SandboxFlag b) {
    return static_cast<SandboxFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(SandboxFlag a, SandboxFlag b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

/**
 * @brief HTML IFrame Element (<iframe>)
 */
class HTMLIFrameElement : public HTMLElement {
public:
    using LoadCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    HTMLIFrameElement();
    ~HTMLIFrameElement() override = default;
    
    // Source
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& url);
    
    std::string srcdoc() const { return getAttribute("srcdoc"); }
    void setSrcdoc(const std::string& html);
    
    // Dimensions  
    std::string width() const { return getAttribute("width"); }
    void setWidth(const std::string& w) { setAttribute("width", w); }
    
    std::string height() const { return getAttribute("height"); }
    void setHeight(const std::string& h) { setAttribute("height", h); }
    
    // Name
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    // Sandbox
    std::string sandbox() const { return getAttribute("sandbox"); }
    void setSandbox(const std::string& s) { setAttribute("sandbox", s); }
    SandboxFlag sandboxFlags() const { return sandboxFlags_; }
    
    // Allow (feature policy)
    std::string allow() const { return getAttribute("allow"); }
    void setAllow(const std::string& a) { setAttribute("allow", a); }
    
    // Referrer policy
    std::string referrerPolicy() const { return getAttribute("referrerpolicy"); }
    void setReferrerPolicy(const std::string& p) { setAttribute("referrerpolicy", p); }
    
    // Loading
    std::string loading() const { return getAttribute("loading"); }  // eager, lazy
    void setLoading(const std::string& l) { setAttribute("loading", l); }
    
    // Content document/window (cross-origin security applies)
    Document* contentDocument() const;
    
    // Callbacks
    void onLoad(LoadCallback cb) { onLoad_ = cb; }
    void onError(ErrorCallback cb) { onError_ = cb; }
    
    std::string tagName() const { return "IFRAME"; }
    
protected:
    void parseAttribute(const std::string& name, const std::string& value);
    
private:
    SandboxFlag sandboxFlags_ = SandboxFlag::None;
    Document* contentDoc_ = nullptr;
    
    LoadCallback onLoad_;
    ErrorCallback onError_;
    
    void parseSandboxAttribute(const std::string& value);
    void loadContent();
};

/**
 * @brief HTML Frame Element (<frame>) - Legacy
 */
class HTMLFrameElement : public HTMLElement {
public:
    HTMLFrameElement();
    ~HTMLFrameElement() override = default;
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& url) { setAttribute("src", url); }
    
    std::string name() const { return getAttribute("name"); }
    void setName(const std::string& n) { setAttribute("name", n); }
    
    bool noResize() const { return hasAttribute("noresize"); }
    void setNoResize(bool r) { if (r) setAttribute("noresize", ""); else removeAttribute("noresize"); }
    
    std::string scrolling() const { return getAttribute("scrolling"); }
    void setScrolling(const std::string& s) { setAttribute("scrolling", s); }
};

/**
 * @brief HTML FrameSet Element (<frameset>) - Legacy
 */
class HTMLFrameSetElement : public HTMLElement {
public:
    HTMLFrameSetElement();
    ~HTMLFrameSetElement() override = default;
    
    std::string cols() const { return getAttribute("cols"); }
    void setCols(const std::string& c) { setAttribute("cols", c); }
    
    std::string rows() const { return getAttribute("rows"); }
    void setRows(const std::string& r) { setAttribute("rows", r); }
};

} // namespace Zepra::WebCore
