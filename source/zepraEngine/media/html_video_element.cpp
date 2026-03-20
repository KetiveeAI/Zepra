// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file html_video_element.cpp
 * @brief HTMLVideoElement implementation
 * 
 * Connects HTML5 <video> API to browser MediaPipeline.
 */

#include "engine/html_video_element.h"
#include "engine/media_pipeline.h"
#include "engine/video_processor.h"
#include "engine/browser_audio.h"

#include <iostream>

namespace zepra {
namespace browser {

class HTMLVideoElement::Impl {
public:
    // Source
    std::string src;
    std::string currentSrc;
    std::string poster;
    
    // Display
    int width = 0;
    int height = 0;
    bool hasControls = true;
    bool autoplay = false;
    bool loop = false;
    
    // Playback
    double defaultPlaybackRate = 1.0;
    
    // State
    bool paused = true;
    bool ended = false;
    VideoReadyState readyState = VideoReadyState::HaveNothing;
    VideoNetworkState networkState = VideoNetworkState::Empty;
    
    // Buffered ranges
    std::vector<TimeRange> buffered;
    std::vector<TimeRange> played;
    
    // Event callbacks
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onEnded;
    std::function<void()> onTimeUpdate;
    std::function<void()> onLoadedMetadata;
    std::function<void()> onLoadedData;
    std::function<void()> onCanPlay;
    std::function<void()> onCanPlayThrough;
    std::function<void()> onWaiting;
    std::function<void()> onSeeking;
    std::function<void()> onSeeked;
    std::function<void()> onProgress;
    std::function<void(const std::string&)> onError;
    std::function<void()> onVolumeChange;
    
    // Current frame texture
    uint32_t currentTexture = 0;
};

HTMLVideoElement::HTMLVideoElement() : impl_(std::make_unique<Impl>()) {
    // Connect to media pipeline events
    auto& pipeline = media::getMediaPipeline();
    
    pipeline.setOnStateChange([this](media::PlaybackState state) {
        switch (state) {
            case media::PlaybackState::Playing:
                impl_->paused = false;
                if (impl_->onPlay) impl_->onPlay();
                break;
            case media::PlaybackState::Paused:
                impl_->paused = true;
                if (impl_->onPause) impl_->onPause();
                break;
            case media::PlaybackState::Stopped:
                impl_->ended = true;
                if (impl_->onEnded) impl_->onEnded();
                break;
            default:
                break;
        }
    });
    
    pipeline.setOnTimeUpdate([this](double time) {
        if (impl_->onTimeUpdate) impl_->onTimeUpdate();
    });
    
    pipeline.setOnEnded([this]() {
        impl_->ended = true;
        if (impl_->loop) {
            // Restart if looping
            media::getMediaPipeline().seek(0);
            media::getMediaPipeline().play();
            impl_->ended = false;
        } else if (impl_->onEnded) {
            impl_->onEnded();
        }
    });
}

HTMLVideoElement::~HTMLVideoElement() = default;

// =============================================================================
// Source
// =============================================================================

void HTMLVideoElement::setSrc(const std::string& url) {
    impl_->src = url;
    impl_->networkState = VideoNetworkState::Loading;
    
    // Trigger load
    load();
}

std::string HTMLVideoElement::getSrc() const {
    return impl_->src;
}

std::string HTMLVideoElement::getCurrentSrc() const {
    return impl_->currentSrc;
}

// =============================================================================
// Playback Control
// =============================================================================

void HTMLVideoElement::play() {
    auto& pipeline = media::getMediaPipeline();
    
    if (!pipeline.isLoaded()) {
        load();
    }
    
    pipeline.play();
    impl_->paused = false;
    impl_->ended = false;
}

void HTMLVideoElement::pause() {
    media::getMediaPipeline().pause();
    impl_->paused = true;
}

void HTMLVideoElement::load() {
    impl_->networkState = VideoNetworkState::Loading;
    impl_->readyState = VideoReadyState::HaveNothing;
    
    auto& pipeline = media::getMediaPipeline();
    
    if (pipeline.load(impl_->src)) {
        impl_->currentSrc = impl_->src;
        
        // Get media info
        auto info = pipeline.getMediaInfo();
        impl_->width = info.videoWidth;
        impl_->height = info.videoHeight;
        
        impl_->readyState = VideoReadyState::HaveMetadata;
        if (impl_->onLoadedMetadata) impl_->onLoadedMetadata();
        
        impl_->readyState = VideoReadyState::HaveEnoughData;
        if (impl_->onCanPlayThrough) impl_->onCanPlayThrough();
        
        impl_->networkState = VideoNetworkState::Idle;
        
        // Autoplay if enabled
        if (impl_->autoplay) {
            play();
        }
    } else {
        impl_->networkState = VideoNetworkState::NoSource;
        if (impl_->onError) impl_->onError("Failed to load video");
    }
}

std::string HTMLVideoElement::canPlayType(const std::string& mimeType) const {
    // Return "probably", "maybe", or ""
    
    // Video formats
    if (mimeType.find("video/mp4") != std::string::npos) return "probably";
    if (mimeType.find("video/webm") != std::string::npos) return "probably";
    if (mimeType.find("video/ogg") != std::string::npos) return "maybe";
    
    // Audio formats
    if (mimeType.find("audio/mp4") != std::string::npos) return "probably";
    if (mimeType.find("audio/mpeg") != std::string::npos) return "probably";
    if (mimeType.find("audio/ogg") != std::string::npos) return "maybe";
    if (mimeType.find("audio/webm") != std::string::npos) return "probably";
    
    // Codecs
    if (mimeType.find("avc1") != std::string::npos) return "probably";  // H.264
    if (mimeType.find("hvc1") != std::string::npos) return "probably";  // H.265
    if (mimeType.find("vp8") != std::string::npos) return "probably";
    if (mimeType.find("vp9") != std::string::npos) return "probably";
    if (mimeType.find("av01") != std::string::npos) return "maybe";     // AV1
    if (mimeType.find("mp4a") != std::string::npos) return "probably";  // AAC
    if (mimeType.find("opus") != std::string::npos) return "probably";
    
    return "";
}

// =============================================================================
// Playback State
// =============================================================================

bool HTMLVideoElement::isPaused() const {
    return impl_->paused;
}

bool HTMLVideoElement::isEnded() const {
    return impl_->ended;
}

bool HTMLVideoElement::isSeeking() const {
    return media::getMediaPipeline().isSeeking();
}

double HTMLVideoElement::getCurrentTime() const {
    return media::getMediaPipeline().getCurrentTime();
}

void HTMLVideoElement::setCurrentTime(double seconds) {
    if (impl_->onSeeking) impl_->onSeeking();
    media::getMediaPipeline().seek(seconds);
    impl_->ended = false;
    if (impl_->onSeeked) impl_->onSeeked();
}

double HTMLVideoElement::getDuration() const {
    return media::getMediaPipeline().getDuration();
}

VideoReadyState HTMLVideoElement::getReadyState() const {
    return impl_->readyState;
}

VideoNetworkState HTMLVideoElement::getNetworkState() const {
    return impl_->networkState;
}

// =============================================================================
// Buffered Ranges
// =============================================================================

std::vector<HTMLVideoElement::TimeRange> HTMLVideoElement::getBuffered() const {
    double buffered = media::getMediaPipeline().getBufferedTime();
    if (buffered > 0) {
        return {{0.0, buffered}};
    }
    return {};
}

std::vector<HTMLVideoElement::TimeRange> HTMLVideoElement::getPlayed() const {
    return impl_->played;
}

std::vector<HTMLVideoElement::TimeRange> HTMLVideoElement::getSeekable() const {
    double duration = getDuration();
    if (duration > 0) {
        return {{0.0, duration}};
    }
    return {};
}

// =============================================================================
// Volume
// =============================================================================

double HTMLVideoElement::getVolume() const {
    return media::getMediaPipeline().getVolume();
}

void HTMLVideoElement::setVolume(double volume) {
    media::getMediaPipeline().setVolume(static_cast<float>(volume));
    if (impl_->onVolumeChange) impl_->onVolumeChange();
}

bool HTMLVideoElement::isMuted() const {
    return media::getMediaPipeline().isMuted();
}

void HTMLVideoElement::setMuted(bool muted) {
    media::getMediaPipeline().setMuted(muted);
    if (impl_->onVolumeChange) impl_->onVolumeChange();
}

// =============================================================================
// Video Dimensions
// =============================================================================

int HTMLVideoElement::getVideoWidth() const {
    auto info = media::getMediaPipeline().getMediaInfo();
    return info.videoWidth;
}

int HTMLVideoElement::getVideoHeight() const {
    auto info = media::getMediaPipeline().getMediaInfo();
    return info.videoHeight;
}

int HTMLVideoElement::getWidth() const {
    return impl_->width > 0 ? impl_->width : getVideoWidth();
}

void HTMLVideoElement::setWidth(int width) {
    impl_->width = width;
}

int HTMLVideoElement::getHeight() const {
    return impl_->height > 0 ? impl_->height : getVideoHeight();
}

void HTMLVideoElement::setHeight(int height) {
    impl_->height = height;
}

// =============================================================================
// Playback Rate
// =============================================================================

double HTMLVideoElement::getPlaybackRate() const {
    return media::getMediaPipeline().getPlaybackRate();
}

void HTMLVideoElement::setPlaybackRate(double rate) {
    media::getMediaPipeline().setPlaybackRate(static_cast<float>(rate));
}

double HTMLVideoElement::getDefaultPlaybackRate() const {
    return impl_->defaultPlaybackRate;
}

void HTMLVideoElement::setDefaultPlaybackRate(double rate) {
    impl_->defaultPlaybackRate = rate;
}

// =============================================================================
// Display
// =============================================================================

std::string HTMLVideoElement::getPoster() const {
    return impl_->poster;
}

void HTMLVideoElement::setPoster(const std::string& url) {
    impl_->poster = url;
}

bool HTMLVideoElement::hasControls() const {
    return impl_->hasControls;
}

void HTMLVideoElement::setControls(bool show) {
    impl_->hasControls = show;
}

bool HTMLVideoElement::isAutoplay() const {
    return impl_->autoplay;
}

void HTMLVideoElement::setAutoplay(bool autoplay) {
    impl_->autoplay = autoplay;
}

bool HTMLVideoElement::isLoop() const {
    return impl_->loop;
}

void HTMLVideoElement::setLoop(bool loop) {
    impl_->loop = loop;
}

// =============================================================================
// Rendering
// =============================================================================

uint32_t HTMLVideoElement::getCurrentFrameTexture() const {
    auto frame = media::getMediaPipeline().getCurrentFrame();
    return frame.textureId;
}

void HTMLVideoElement::update() {
    media::getMediaPipeline().update();
}

void HTMLVideoElement::render(int x, int y, int width, int height) {
    // Get current frame and render
    // This would be called by the browser's render loop
    uint32_t texture = getCurrentFrameTexture();
    if (texture) {
        // Rendering is handled by the pipeline's renderer
        // Here we just ensure the frame is ready
    }
}

// =============================================================================
// Events
// =============================================================================

void HTMLVideoElement::setOnPlay(std::function<void()> callback) {
    impl_->onPlay = callback;
}

void HTMLVideoElement::setOnPause(std::function<void()> callback) {
    impl_->onPause = callback;
}

void HTMLVideoElement::setOnEnded(std::function<void()> callback) {
    impl_->onEnded = callback;
}

void HTMLVideoElement::setOnTimeUpdate(std::function<void()> callback) {
    impl_->onTimeUpdate = callback;
}

void HTMLVideoElement::setOnLoadedMetadata(std::function<void()> callback) {
    impl_->onLoadedMetadata = callback;
}

void HTMLVideoElement::setOnLoadedData(std::function<void()> callback) {
    impl_->onLoadedData = callback;
}

void HTMLVideoElement::setOnCanPlay(std::function<void()> callback) {
    impl_->onCanPlay = callback;
}

void HTMLVideoElement::setOnCanPlayThrough(std::function<void()> callback) {
    impl_->onCanPlayThrough = callback;
}

void HTMLVideoElement::setOnWaiting(std::function<void()> callback) {
    impl_->onWaiting = callback;
}

void HTMLVideoElement::setOnSeeking(std::function<void()> callback) {
    impl_->onSeeking = callback;
}

void HTMLVideoElement::setOnSeeked(std::function<void()> callback) {
    impl_->onSeeked = callback;
}

void HTMLVideoElement::setOnProgress(std::function<void()> callback) {
    impl_->onProgress = callback;
}

void HTMLVideoElement::setOnError(std::function<void(const std::string&)> callback) {
    impl_->onError = callback;
}

void HTMLVideoElement::setOnVolumeChange(std::function<void()> callback) {
    impl_->onVolumeChange = callback;
}

} // namespace browser
} // namespace zepra
