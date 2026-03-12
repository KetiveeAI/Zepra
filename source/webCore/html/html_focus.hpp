/**
 * @file html_focus.hpp
 * @brief Focus management APIs
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Focus options
 */
struct FocusOptions {
    bool preventScroll = false;
    std::string focusVisible;  // auto, manual
};

/**
 * @brief Focus event
 */
struct FocusEvent {
    HTMLElement* target = nullptr;
    HTMLElement* relatedTarget = nullptr;
};

/**
 * @brief Focusable mixin
 */
class Focusable {
public:
    virtual ~Focusable() = default;
    
    int tabIndex() const { return tabIndex_; }
    void setTabIndex(int i) { tabIndex_ = i; }
    
    void focus(const FocusOptions& options = {});
    void blur();
    
    bool hasFocus() const { return focused_; }
    
    std::function<void(const FocusEvent&)> onFocus;
    std::function<void(const FocusEvent&)> onBlur;
    std::function<void(const FocusEvent&)> onFocusIn;
    std::function<void(const FocusEvent&)> onFocusOut;
    
protected:
    int tabIndex_ = -1;
    bool focused_ = false;
};

/**
 * @brief Focus trap for modals
 */
class FocusTrap {
public:
    FocusTrap(HTMLElement* container);
    ~FocusTrap();
    
    void activate();
    void deactivate();
    
    bool isActive() const { return active_; }
    
private:
    HTMLElement* container_;
    bool active_ = false;
    HTMLElement* previousFocus_ = nullptr;
    std::vector<HTMLElement*> focusableElements_;
    
    void updateFocusableElements();
    void handleKeyDown(int keyCode);
};

/**
 * @brief Focus visible state
 */
class FocusVisible {
public:
    static bool shouldShowFocusRing(HTMLElement* element);
    static void setHadKeyboardEvent(bool had);
    
private:
    static bool hadKeyboardEvent_;
};

/**
 * @brief Inert attribute support
 */
class Inert {
public:
    static bool isInert(HTMLElement* element);
    static void setInert(HTMLElement* element, bool inert);
};

} // namespace Zepra::WebCore
