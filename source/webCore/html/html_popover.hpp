/**
 * @file html_popover.hpp
 * @brief Popover API (new HTML spec)
 */

#pragma once

#include "html/html_element.hpp"
#include <string>

namespace Zepra::WebCore {

/**
 * @brief Popover state
 */
enum class PopoverState {
    Hidden,
    Showing
};

/**
 * @brief Popover API mixin for elements
 */
class PopoverElement {
public:
    virtual ~PopoverElement() = default;
    
    std::string popover() const { return popover_; }
    void setPopover(const std::string& p) { popover_ = p; }
    
    void showPopover();
    void hidePopover();
    void togglePopover(bool force = false);
    
    bool matchesPopoverState(PopoverState state) const;
    
protected:
    std::string popover_;  // auto, manual
    PopoverState state_ = PopoverState::Hidden;
};

/**
 * @brief Popover invoker target
 */
class PopoverInvokerElement {
public:
    virtual ~PopoverInvokerElement() = default;
    
    HTMLElement* popoverTargetElement() const { return popoverTarget_; }
    void setPopoverTargetElement(HTMLElement* el) { popoverTarget_ = el; }
    
    std::string popoverTargetAction() const { return popoverAction_; }
    void setPopoverTargetAction(const std::string& a) { popoverAction_ = a; }
    
protected:
    HTMLElement* popoverTarget_ = nullptr;
    std::string popoverAction_ = "toggle";  // toggle, show, hide
};

} // namespace Zepra::WebCore
