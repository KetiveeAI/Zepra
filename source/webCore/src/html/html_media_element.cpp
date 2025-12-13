/**
 * @file html_media_element.cpp
 * @brief HTMLMediaElement, HTMLVideoElement, HTMLAudioElement implementation
 *
 * Full media playback implementation.
 */

#include "webcore/html/html_media_element.hpp"
#include "webcore/html/html_anchor_element.hpp"  // For DOMTokenList
#include <cmath>
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// HTMLMediaElement::Impl
// =============================================================================

class HTMLMediaElement::Impl {
public:
    // State
    MediaNetworkState networkState = MediaNetworkState::Empty;
    MediaReadyState readyState = MediaReadyState::HaveNothing;
    
    // Playback state
    double currentTime = 0.0;
    double duration = std::nan("");
    bool paused = true;
    bool ended = false;
    bool seeking = false;
    
    // Volume/mute
    double volume = 1.0;
    bool muted = false;
    
    // Playback rate
    double playbackRate = 1.0;
    double defaultPlaybackRate = 1.0;
    bool preservesPitch = true;
    
    // Time ranges
    TimeRanges buffered;
    TimeRanges played;
    TimeRanges seekable;
    
    // Error
    std::unique_ptr<MediaError> error;
    
    // Controls list
    std::unique_ptr<DOMTokenList> controlsList;
    
    // Event handlers
    std::unordered_map<std::string, EventListener> eventHandlers;
};

// =============================================================================
// HTMLMediaElement
// =============================================================================

HTMLMediaElement::HTMLMediaElement(const std::string& tagName)
    : HTMLElement(tagName),
      impl_(std::make_unique<Impl>()) {
    impl_->controlsList = std::make_unique<DOMTokenList>(this, "controlslist");
}

HTMLMediaElement::~HTMLMediaElement() = default;

// Source properties

std::string HTMLMediaElement::src() const {
    return getAttribute("src");
}

void HTMLMediaElement::setSrc(const std::string& src) {
    setAttribute("src", src);
    // Trigger load
    load();
}

std::string HTMLMediaElement::currentSrc() const {
    std::string s = src();
    if (!s.empty()) return s;
    
    // Check <source> children
    for (const auto& child : childNodes()) {
        if (auto* elem = dynamic_cast<HTMLElement*>(child.get())) {
            if (elem->tagName() == "source" || elem->tagName() == "SOURCE") {
                return elem->getAttribute("src");
            }
        }
    }
    
    return "";
}

std::string HTMLMediaElement::crossOrigin() const {
    if (!hasAttribute("crossorigin")) return "";
    std::string val = getAttribute("crossorigin");
    if (val == "use-credentials") return "use-credentials";
    return "anonymous";
}

void HTMLMediaElement::setCrossOrigin(const std::string& mode) {
    if (mode.empty()) {
        removeAttribute("crossorigin");
    } else {
        setAttribute("crossorigin", mode);
    }
}

// Playback state

double HTMLMediaElement::currentTime() const {
    return impl_->currentTime;
}

void HTMLMediaElement::setCurrentTime(double time) {
    if (std::isnan(time)) return;
    
    impl_->seeking = true;
    
    // Fire seeking event
    Event seekingEvent("seeking", true, false);
    dispatchEvent(seekingEvent);
    
    impl_->currentTime = std::max(0.0, std::min(time, duration()));
    impl_->seeking = false;
    
    // Fire seeked event
    Event seekedEvent("seeked", true, false);
    dispatchEvent(seekedEvent);
    
    // Fire timeupdate
    Event timeUpdateEvent("timeupdate", true, false);
    dispatchEvent(timeUpdateEvent);
}

double HTMLMediaElement::duration() const {
    return impl_->duration;
}

bool HTMLMediaElement::ended() const {
    return impl_->ended;
}

bool HTMLMediaElement::paused() const {
    return impl_->paused;
}

bool HTMLMediaElement::seeking() const {
    return impl_->seeking;
}

// Network state

MediaNetworkState HTMLMediaElement::networkState() const {
    return impl_->networkState;
}

MediaReadyState HTMLMediaElement::readyState() const {
    return impl_->readyState;
}

TimeRanges* HTMLMediaElement::buffered() {
    return &impl_->buffered;
}

const TimeRanges* HTMLMediaElement::buffered() const {
    return &impl_->buffered;
}

TimeRanges* HTMLMediaElement::played() {
    return &impl_->played;
}

const TimeRanges* HTMLMediaElement::played() const {
    return &impl_->played;
}

TimeRanges* HTMLMediaElement::seekable() {
    return &impl_->seekable;
}

const TimeRanges* HTMLMediaElement::seekable() const {
    return &impl_->seekable;
}

std::string HTMLMediaElement::preload() const {
    std::string val = getAttribute("preload");
    if (val == "none") return "none";
    if (val == "metadata") return "metadata";
    return "auto";
}

void HTMLMediaElement::setPreload(const std::string& preload) {
    setAttribute("preload", preload);
}

// Playback control

bool HTMLMediaElement::autoplay() const {
    return hasAttribute("autoplay");
}

void HTMLMediaElement::setAutoplay(bool autoplay) {
    if (autoplay) {
        setAttribute("autoplay", "");
    } else {
        removeAttribute("autoplay");
    }
}

bool HTMLMediaElement::loop() const {
    return hasAttribute("loop");
}

void HTMLMediaElement::setLoop(bool loop) {
    if (loop) {
        setAttribute("loop", "");
    } else {
        removeAttribute("loop");
    }
}

bool HTMLMediaElement::controls() const {
    return hasAttribute("controls");
}

void HTMLMediaElement::setControls(bool controls) {
    if (controls) {
        setAttribute("controls", "");
    } else {
        removeAttribute("controls");
    }
}

DOMTokenList* HTMLMediaElement::controlsList() {
    return impl_->controlsList.get();
}

double HTMLMediaElement::volume() const {
    return impl_->volume;
}

void HTMLMediaElement::setVolume(double volume) {
    volume = std::max(0.0, std::min(1.0, volume));
    if (impl_->volume != volume) {
        impl_->volume = volume;
        
        // Fire volumechange event
        Event event("volumechange", true, false);
        dispatchEvent(event);
    }
}

bool HTMLMediaElement::muted() const {
    return impl_->muted;
}

void HTMLMediaElement::setMuted(bool muted) {
    if (impl_->muted != muted) {
        impl_->muted = muted;
        
        // Fire volumechange event
        Event event("volumechange", true, false);
        dispatchEvent(event);
    }
}

bool HTMLMediaElement::defaultMuted() const {
    return hasAttribute("muted");
}

void HTMLMediaElement::setDefaultMuted(bool muted) {
    if (muted) {
        setAttribute("muted", "");
    } else {
        removeAttribute("muted");
    }
}

double HTMLMediaElement::playbackRate() const {
    return impl_->playbackRate;
}

void HTMLMediaElement::setPlaybackRate(double rate) {
    if (impl_->playbackRate != rate) {
        impl_->playbackRate = rate;
        
        // Fire ratechange event
        Event event("ratechange", true, false);
        dispatchEvent(event);
    }
}

double HTMLMediaElement::defaultPlaybackRate() const {
    return impl_->defaultPlaybackRate;
}

void HTMLMediaElement::setDefaultPlaybackRate(double rate) {
    impl_->defaultPlaybackRate = rate;
}

bool HTMLMediaElement::preservesPitch() const {
    return impl_->preservesPitch;
}

void HTMLMediaElement::setPreservesPitch(bool preserve) {
    impl_->preservesPitch = preserve;
}

// Error

const MediaError* HTMLMediaElement::error() const {
    return impl_->error.get();
}

// Methods

void HTMLMediaElement::play() {
    if (!impl_->paused) return;
    
    impl_->paused = false;
    impl_->ended = false;
    
    // Fire play event
    Event playEvent("play", true, false);
    dispatchEvent(playEvent);
    
    // Fire playing event
    Event playingEvent("playing", true, false);
    dispatchEvent(playingEvent);
    
    // In production, would interface with media pipeline
    // to start actual playback
}

void HTMLMediaElement::pause() {
    if (impl_->paused) return;
    
    impl_->paused = true;
    
    // Fire pause event
    Event event("pause", true, false);
    dispatchEvent(event);
}

void HTMLMediaElement::load() {
    // Reset state
    impl_->currentTime = 0.0;
    impl_->paused = true;
    impl_->ended = false;
    impl_->seeking = false;
    impl_->error.reset();
    impl_->networkState = MediaNetworkState::Loading;
    impl_->readyState = MediaReadyState::HaveNothing;
    
    impl_->buffered = TimeRanges();
    impl_->played = TimeRanges();
    impl_->seekable = TimeRanges();
    
    // Fire loadstart event
    Event event("loadstart", true, false);
    dispatchEvent(event);
    
    // In production, would begin resource fetch
    // and update ready state as metadata/data loads
}

void HTMLMediaElement::fastSeek(double time) {
    // Fast seek with less precision
    setCurrentTime(time);
}

std::string HTMLMediaElement::canPlayType(const std::string& mediaType) const {
    // Common supported types
    if (mediaType.find("video/mp4") != std::string::npos ||
        mediaType.find("video/webm") != std::string::npos ||
        mediaType.find("audio/mp3") != std::string::npos ||
        mediaType.find("audio/mpeg") != std::string::npos ||
        mediaType.find("audio/ogg") != std::string::npos ||
        mediaType.find("audio/wav") != std::string::npos) {
        return "probably";
    }
    
    if (mediaType.find("video/") != std::string::npos ||
        mediaType.find("audio/") != std::string::npos) {
        return "maybe";
    }
    
    return "";
}

// Event handlers

#define DEFINE_MEDIA_EVENT_HANDLER(name, eventName) \
    void HTMLMediaElement::setOn##name(EventListener callback) { \
        impl_->eventHandlers[eventName] = std::move(callback); \
        addEventListener(eventName, impl_->eventHandlers[eventName]); \
    }

DEFINE_MEDIA_EVENT_HANDLER(Play, "play")
DEFINE_MEDIA_EVENT_HANDLER(Pause, "pause")
DEFINE_MEDIA_EVENT_HANDLER(Playing, "playing")
DEFINE_MEDIA_EVENT_HANDLER(Ended, "ended")
DEFINE_MEDIA_EVENT_HANDLER(TimeUpdate, "timeupdate")
DEFINE_MEDIA_EVENT_HANDLER(DurationChange, "durationchange")
DEFINE_MEDIA_EVENT_HANDLER(VolumeChange, "volumechange")
DEFINE_MEDIA_EVENT_HANDLER(LoadStart, "loadstart")
DEFINE_MEDIA_EVENT_HANDLER(Progress, "progress")
DEFINE_MEDIA_EVENT_HANDLER(Suspend, "suspend")
DEFINE_MEDIA_EVENT_HANDLER(Abort, "abort")
DEFINE_MEDIA_EVENT_HANDLER(Error, "error")
DEFINE_MEDIA_EVENT_HANDLER(Emptied, "emptied")
DEFINE_MEDIA_EVENT_HANDLER(Stalled, "stalled")
DEFINE_MEDIA_EVENT_HANDLER(LoadedMetadata, "loadedmetadata")
DEFINE_MEDIA_EVENT_HANDLER(LoadedData, "loadeddata")
DEFINE_MEDIA_EVENT_HANDLER(CanPlay, "canplay")
DEFINE_MEDIA_EVENT_HANDLER(CanPlayThrough, "canplaythrough")
DEFINE_MEDIA_EVENT_HANDLER(Seeking, "seeking")
DEFINE_MEDIA_EVENT_HANDLER(Seeked, "seeked")
DEFINE_MEDIA_EVENT_HANDLER(Waiting, "waiting")
DEFINE_MEDIA_EVENT_HANDLER(RateChange, "ratechange")

#undef DEFINE_MEDIA_EVENT_HANDLER

void HTMLMediaElement::copyMediaElementProperties(HTMLMediaElement* target) const {
    copyHTMLElementProperties(target);
    
    target->impl_->volume = impl_->volume;
    target->impl_->muted = impl_->muted;
    target->impl_->playbackRate = impl_->playbackRate;
    target->impl_->defaultPlaybackRate = impl_->defaultPlaybackRate;
    target->impl_->preservesPitch = impl_->preservesPitch;
}

// =============================================================================
// HTMLVideoElement
// =============================================================================

class HTMLVideoElement::VideoImpl {
public:
    unsigned int videoWidth = 0;
    unsigned int videoHeight = 0;
};

HTMLVideoElement::HTMLVideoElement()
    : HTMLMediaElement("video"),
      videoImpl_(std::make_unique<VideoImpl>()) {}

HTMLVideoElement::~HTMLVideoElement() = default;

unsigned int HTMLVideoElement::width() const {
    std::string val = getAttribute("width");
    if (val.empty()) return 0;
    try {
        return static_cast<unsigned int>(std::stoul(val));
    } catch (...) {
        return 0;
    }
}

void HTMLVideoElement::setWidth(unsigned int width) {
    setAttribute("width", std::to_string(width));
}

unsigned int HTMLVideoElement::height() const {
    std::string val = getAttribute("height");
    if (val.empty()) return 0;
    try {
        return static_cast<unsigned int>(std::stoul(val));
    } catch (...) {
        return 0;
    }
}

void HTMLVideoElement::setHeight(unsigned int height) {
    setAttribute("height", std::to_string(height));
}

unsigned int HTMLVideoElement::videoWidth() const {
    return videoImpl_->videoWidth;
}

unsigned int HTMLVideoElement::videoHeight() const {
    return videoImpl_->videoHeight;
}

std::string HTMLVideoElement::poster() const {
    return getAttribute("poster");
}

void HTMLVideoElement::setPoster(const std::string& poster) {
    setAttribute("poster", poster);
}

bool HTMLVideoElement::disablePictureInPicture() const {
    return hasAttribute("disablepictureinpicture");
}

void HTMLVideoElement::setDisablePictureInPicture(bool disabled) {
    if (disabled) {
        setAttribute("disablepictureinpicture", "");
    } else {
        removeAttribute("disablepictureinpicture");
    }
}

bool HTMLVideoElement::playsInline() const {
    return hasAttribute("playsinline");
}

void HTMLVideoElement::setPlaysInline(bool inline_) {
    if (inline_) {
        setAttribute("playsinline", "");
    } else {
        removeAttribute("playsinline");
    }
}

void HTMLVideoElement::requestPictureInPicture() {
    // In production, would trigger PiP mode
    Event event("enterpictureinpicture", true, false);
    dispatchEvent(event);
}

std::unique_ptr<DOMNode> HTMLVideoElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLVideoElement>();
    copyMediaElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

// =============================================================================
// HTMLAudioElement
// =============================================================================

HTMLAudioElement::HTMLAudioElement()
    : HTMLMediaElement("audio") {}

HTMLAudioElement::~HTMLAudioElement() = default;

std::unique_ptr<DOMNode> HTMLAudioElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAudioElement>();
    copyMediaElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

} // namespace Zepra::WebCore
