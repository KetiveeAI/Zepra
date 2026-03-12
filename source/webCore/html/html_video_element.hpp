/**
 * @file html_video_element.hpp
 * @brief HTML Video Element (extends media element)
 * 
 * HTMLAudioElement is in html_audio_element.hpp
 * HTMLMediaElement base is in html_media_element.hpp
 */

#pragma once

#include "html/html_media_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTML Video Element (<video>)
 */
class HTMLVideoElement : public HTMLMediaElement {
public:
    using ResizeCallback = std::function<void(int width, int height)>;
    
    HTMLVideoElement();
    ~HTMLVideoElement() override = default;
    
    // Dimensions
    int width() const { return width_; }
    void setWidth(int w) { width_ = w; }
    
    int height() const { return height_; }
    void setHeight(int h) { height_ = h; }
    
    // Actual video size
    int videoWidth() const { return videoWidth_; }
    int videoHeight() const { return videoHeight_; }
    
    // Poster image
    std::string poster() const { return getAttribute("poster"); }
    void setPoster(const std::string& p) { setAttribute("poster", p); }
    
    // Playback quality
    bool playsInline() const { return hasAttribute("playsinline"); }
    void setPlaysInline(bool p) { if (p) setAttribute("playsinline", ""); else removeAttribute("playsinline"); }
    
    // Picture-in-Picture
    bool disablePictureInPicture() const { return hasAttribute("disablepictureinpicture"); }
    void setDisablePictureInPicture(bool d) { if (d) setAttribute("disablepictureinpicture", ""); else removeAttribute("disablepictureinpicture"); }
    
    // Callbacks
    void onResize(ResizeCallback cb) { onResize_ = cb; }
    
    std::string tagName() const { return "VIDEO"; }
    
private:
    int width_ = 300;
    int height_ = 150;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    
    ResizeCallback onResize_;
};

// Note: HTMLAudioElement is defined in html_audio_element.hpp

} // namespace Zepra::WebCore
