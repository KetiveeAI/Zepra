/**
 * @file html_interactive_element.hpp
 * @brief HTML Interactive elements - details, summary, dialog extensions
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTML Datalist Element (<datalist>)
 */
class HTMLDataListElement : public HTMLElement {
public:
    HTMLDataListElement();
    ~HTMLDataListElement() override = default;
    
    std::vector<HTMLElement*> options() const;
    
    std::string tagName() const { return "DATALIST"; }
};

/**
 * @brief HTML Marquee Element (<marquee>) - Deprecated but still needed
 */
class HTMLMarqueeElement : public HTMLElement {
public:
    HTMLMarqueeElement();
    ~HTMLMarqueeElement() override = default;
    
    std::string behavior() const { return getAttribute("behavior"); }
    void setBehavior(const std::string& b) { setAttribute("behavior", b); }
    
    std::string direction() const { return getAttribute("direction"); }
    void setDirection(const std::string& d) { setAttribute("direction", d); }
    
    int scrollAmount() const { return scrollAmount_; }
    void setScrollAmount(int a) { scrollAmount_ = a; }
    
    int scrollDelay() const { return scrollDelay_; }
    void setScrollDelay(int d) { scrollDelay_ = d; }
    
    int loop() const { return loop_; }
    void setLoop(int l) { loop_ = l; }
    
    void start();
    void stop();
    
    std::string tagName() const { return "MARQUEE"; }
    
private:
    int scrollAmount_ = 6;
    int scrollDelay_ = 85;
    int loop_ = -1;
    bool running_ = true;
};

/**
 * @brief HTML Menu Item Element (<menuitem>) - Deprecated
 */
class HTMLMenuItemElement : public HTMLElement {
public:
    HTMLMenuItemElement();
    ~HTMLMenuItemElement() override = default;
    
    std::string type() const { return getAttribute("type"); }
    void setType(const std::string& t) { setAttribute("type", t); }
    
    std::string label() const { return getAttribute("label"); }
    void setLabel(const std::string& l) { setAttribute("label", l); }
    
    std::string icon() const { return getAttribute("icon"); }
    void setIcon(const std::string& i) { setAttribute("icon", i); }
    
    bool disabled() const { return hasAttribute("disabled"); }
    void setDisabled(bool d) { if (d) setAttribute("disabled", ""); else removeAttribute("disabled"); }
    
    bool checked() const { return hasAttribute("checked"); }
    void setChecked(bool c) { if (c) setAttribute("checked", ""); else removeAttribute("checked"); }
    
    std::string tagName() const { return "MENUITEM"; }
};

/**
 * @brief HTML NoScript Element (<noscript>)
 */
class HTMLNoScriptElement : public HTMLElement {
public:
    HTMLNoScriptElement();
    ~HTMLNoScriptElement() override = default;
    
    std::string tagName() const { return "NOSCRIPT"; }
};

/**
 * @brief HTML Portal Element (<portal>) - Experimental
 */
class HTMLPortalElement : public HTMLElement {
public:
    using ActivateCallback = std::function<void()>;
    
    HTMLPortalElement();
    ~HTMLPortalElement() override = default;
    
    std::string src() const { return getAttribute("src"); }
    void setSrc(const std::string& s) { setAttribute("src", s); }
    
    std::string referrerPolicy() const { return getAttribute("referrerpolicy"); }
    void setReferrerPolicy(const std::string& p) { setAttribute("referrerpolicy", p); }
    
    void activate();
    void onActivate(ActivateCallback cb) { onActivate_ = cb; }
    
    std::string tagName() const { return "PORTAL"; }
    
private:
    ActivateCallback onActivate_;
};

} // namespace Zepra::WebCore
