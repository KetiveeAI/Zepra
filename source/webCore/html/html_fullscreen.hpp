/**
 * @file html_fullscreen.hpp
 * @brief Fullscreen API
 */

#pragma once

#include "html/html_element.hpp"
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Fullscreen options
 */
struct FullscreenOptions {
    std::string navigationUI = "auto";  // auto, show, hide
};

/**
 * @brief Fullscreen API mixin
 */
class FullscreenElement {
public:
    virtual ~FullscreenElement() = default;
    
    void requestFullscreen(const FullscreenOptions& options = {});
    
    virtual HTMLElement* asElement() = 0;
};

/**
 * @brief Document fullscreen API
 */
class FullscreenDocument {
public:
    bool fullscreenEnabled() const { return fullscreenEnabled_; }
    HTMLElement* fullscreenElement() const { return fullscreenElement_; }
    
    void exitFullscreen();
    
    std::function<void()> onFullscreenChange;
    std::function<void()> onFullscreenError;
    
private:
    bool fullscreenEnabled_ = true;
    HTMLElement* fullscreenElement_ = nullptr;
};

} // namespace Zepra::WebCore
