// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_HTML_VIDEO_ELEMENT_H
#define ZEPRA_HTML_VIDEO_ELEMENT_H

/**
 * @file html_video_element.h
 * @brief HTML5 <video> element implementation for browser
 * 
 * Provides HTMLVideoElement API that connects to MediaPipeline
 * for actual playback using the browser's audio/video systems.
 */

#include <string>
#include <functional>
#include <memory>
#include <cstdint>

namespace zepra {
namespace browser {

/**
 * @brief Video playback state
 */
enum class VideoReadyState {
    HaveNothing = 0,
    HaveMetadata = 1,
    HaveCurrentData = 2,
    HaveFutureData = 3,
    HaveEnoughData = 4
};

/**
 * @brief Network state
 */
enum class VideoNetworkState {
    Empty = 0,
    Idle = 1,
    Loading = 2,
    NoSource = 3
};

/**
 * @brief HTMLVideoElement implementation
 * 
 * Implements the HTML5 video element API for browser integration.
 * Connects to MediaPipeline for actual playback.
 */
class HTMLVideoElement {
public:
    HTMLVideoElement();
    ~HTMLVideoElement();
    
    // ==========================================================================
    // Source
    // ==========================================================================
    
    /**
     * @brief Set video source URL
     */
    void setSrc(const std::string& url);
    std::string getSrc() const;
    
    /**
     * @brief Get current source URL being used
     */
    std::string getCurrentSrc() const;
    
    // ==========================================================================
    // Playback Control
    // ==========================================================================
    
    void play();
    void pause();
    void load();
    
    /**
     * @brief Check if can play this MIME type
     */
    std::string canPlayType(const std::string& mimeType) const;
    
    // ==========================================================================
    // Playback State
    // ==========================================================================
    
    bool isPaused() const;
    bool isEnded() const;
    bool isSeeking() const;
    
    double getCurrentTime() const;
    void setCurrentTime(double seconds);
    
    double getDuration() const;
    
    VideoReadyState getReadyState() const;
    VideoNetworkState getNetworkState() const;
    
    // ==========================================================================
    // Buffered Ranges
    // ==========================================================================
    
    struct TimeRange {
        double start;
        double end;
    };
    
    std::vector<TimeRange> getBuffered() const;
    std::vector<TimeRange> getPlayed() const;
    std::vector<TimeRange> getSeekable() const;
    
    // ==========================================================================
    // Volume
    // ==========================================================================
    
    double getVolume() const;
    void setVolume(double volume);
    
    bool isMuted() const;
    void setMuted(bool muted);
    
    // ==========================================================================
    // Video Dimensions
    // ==========================================================================
    
    int getVideoWidth() const;
    int getVideoHeight() const;
    
    int getWidth() const;
    void setWidth(int width);
    
    int getHeight() const;
    void setHeight(int height);
    
    // ==========================================================================
    // Playback Rate
    // ==========================================================================
    
    double getPlaybackRate() const;
    void setPlaybackRate(double rate);
    
    double getDefaultPlaybackRate() const;
    void setDefaultPlaybackRate(double rate);
    
    // ==========================================================================
    // Display
    // ==========================================================================
    
    std::string getPoster() const;
    void setPoster(const std::string& url);
    
    bool hasControls() const;
    void setControls(bool show);
    
    bool isAutoplay() const;
    void setAutoplay(bool autoplay);
    
    bool isLoop() const;
    void setLoop(bool loop);
    
    // ==========================================================================
    // Rendering
    // ==========================================================================
    
    /**
     * @brief Get current video frame for rendering
     * @return OpenGL texture ID
     */
    uint32_t getCurrentFrameTexture() const;
    
    /**
     * @brief Update playback (call from render loop)
     */
    void update();
    
    /**
     * @brief Render video at specified position
     */
    void render(int x, int y, int width, int height);
    
    // ==========================================================================
    // Events (for JavaScript binding)
    // ==========================================================================
    
    void setOnPlay(std::function<void()> callback);
    void setOnPause(std::function<void()> callback);
    void setOnEnded(std::function<void()> callback);
    void setOnTimeUpdate(std::function<void()> callback);
    void setOnLoadedMetadata(std::function<void()> callback);
    void setOnLoadedData(std::function<void()> callback);
    void setOnCanPlay(std::function<void()> callback);
    void setOnCanPlayThrough(std::function<void()> callback);
    void setOnWaiting(std::function<void()> callback);
    void setOnSeeking(std::function<void()> callback);
    void setOnSeeked(std::function<void()> callback);
    void setOnProgress(std::function<void()> callback);
    void setOnError(std::function<void(const std::string&)> callback);
    void setOnVolumeChange(std::function<void()> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace browser
} // namespace zepra

#endif // ZEPRA_HTML_VIDEO_ELEMENT_H
