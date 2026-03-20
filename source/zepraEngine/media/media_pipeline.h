// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_MEDIA_PIPELINE_H
#define ZEPRA_MEDIA_PIPELINE_H

/**
 * @file media_pipeline.h
 * @brief Unified media pipeline connecting audio/video to renderer
 * 
 * Integrates:
 * - Video decoding (HW and SW)
 * - Audio decoding and processing
 * - Audio/Video synchronization
 * - OpenGL texture upload for video frames
 * - Audio output via NXAudio
 */

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace zepra {
namespace media {

// Forward declarations
class AudioDecoder;
class VideoDecoder;

/**
 * @brief Media playback state
 */
enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
    Buffering,
    Error
};

/**
 * @brief Decoded video frame
 */
struct VideoFrame {
    uint8_t* data = nullptr;      // RGB or YUV data
    int width = 0;
    int height = 0;
    int stride = 0;               // Bytes per row
    double pts = 0.0;             // Presentation timestamp (seconds)
    bool isKeyFrame = false;
    
    // For hardware decode
    bool isHardwareFrame = false;
    uint32_t textureId = 0;       // GL texture if hardware
};

/**
 * @brief Decoded audio samples
 */
struct AudioSamples {
    float* data = nullptr;        // Interleaved stereo float
    size_t frameCount = 0;        // Number of stereo frames
    double pts = 0.0;             // Presentation timestamp
    int sampleRate = 48000;
    int channels = 2;
};

/**
 * @brief Media information
 */
struct MediaInfo {
    // Video
    bool hasVideo = false;
    int videoWidth = 0;
    int videoHeight = 0;
    float videoFPS = 0.0f;
    std::string videoCodec;
    
    // Audio
    bool hasAudio = false;
    int audioSampleRate = 0;
    int audioChannels = 0;
    std::string audioCodec;
    
    // General
    double duration = 0.0;
    std::string container;
    std::string title;
};

/**
 * @brief Rendering callback interface
 */
class MediaRenderer {
public:
    virtual ~MediaRenderer() = default;
    
    /**
     * @brief Upload video frame to GPU texture
     * @param frame Decoded video frame
     * @return OpenGL texture ID
     */
    virtual uint32_t uploadFrame(const VideoFrame& frame) = 0;
    
    /**
     * @brief Render video texture to screen
     * @param textureId OpenGL texture
     * @param x Position X
     * @param y Position Y
     * @param width Render width
     * @param height Render height
     */
    virtual void renderFrame(uint32_t textureId, int x, int y, int width, int height) = 0;
    
    /**
     * @brief Present frame (swap buffers)
     */
    virtual void present() = 0;
};

/**
 * @brief Audio output interface
 */
class AudioOutput {
public:
    virtual ~AudioOutput() = default;
    
    /**
     * @brief Initialize audio output
     */
    virtual bool initialize(int sampleRate, int channels) = 0;
    
    /**
     * @brief Queue audio samples for playback
     */
    virtual void queueSamples(const AudioSamples& samples) = 0;
    
    /**
     * @brief Get current playback position
     */
    virtual double getPlaybackPosition() const = 0;
    
    /**
     * @brief Pause/resume audio
     */
    virtual void pause(bool paused) = 0;
    
    /**
     * @brief Clear audio queue
     */
    virtual void flush() = 0;
};

/**
 * @brief Unified media playback pipeline
 */
class MediaPipeline {
public:
    MediaPipeline();
    ~MediaPipeline();
    
    // ==========================================================================
    // Initialization
    // ==========================================================================
    
    /**
     * @brief Set renderer for video output
     */
    void setRenderer(MediaRenderer* renderer);
    
    /**
     * @brief Set audio output
     */
    void setAudioOutput(AudioOutput* output);
    
    // ==========================================================================
    // Media Loading
    // ==========================================================================
    
    /**
     * @brief Load media from URL or file
     * @param url Media URL (http, file, etc.)
     * @return true if loaded successfully
     */
    bool load(const std::string& url);
    
    /**
     * @brief Load from raw data
     */
    bool loadFromMemory(const uint8_t* data, size_t size, const std::string& mimeType);
    
    /**
     * @brief Get media information
     */
    MediaInfo getMediaInfo() const;
    
    /**
     * @brief Check if media is loaded
     */
    bool isLoaded() const;
    
    // ==========================================================================
    // Playback Control
    // ==========================================================================
    
    void play();
    void pause();
    void stop();
    void togglePlayPause();
    
    /**
     * @brief Seek to position
     * @param seconds Target position in seconds
     */
    void seek(double seconds);
    
    // ==========================================================================
    // Playback State
    // ==========================================================================
    
    PlaybackState getState() const;
    double getCurrentTime() const;
    double getDuration() const;
    double getBufferedTime() const;
    
    bool isPlaying() const;
    bool isPaused() const;
    bool isSeeking() const;
    
    // ==========================================================================
    // Audio Control
    // ==========================================================================
    
    void setVolume(float volume);
    float getVolume() const;
    
    void setMuted(bool muted);
    bool isMuted() const;
    
    // ==========================================================================
    // Video Control
    // ==========================================================================
    
    /**
     * @brief Get current video frame (for external rendering)
     */
    VideoFrame getCurrentFrame();
    
    /**
     * @brief Set playback rate
     */
    void setPlaybackRate(float rate);
    float getPlaybackRate() const;
    
    // ==========================================================================
    // Processing Settings
    // ==========================================================================
    
    /**
     * @brief Enable/disable hardware decode
     */
    void setHardwareDecodeEnabled(bool enabled);
    bool isHardwareDecodeEnabled() const;
    
    /**
     * @brief Enable audio processing (equalizer, etc.)
     */
    void setAudioProcessingEnabled(bool enabled);
    
    /**
     * @brief Enable video processing (adjustments, etc.)
     */
    void setVideoProcessingEnabled(bool enabled);
    
    // ==========================================================================
    // Callbacks
    // ==========================================================================
    
    void setOnStateChange(std::function<void(PlaybackState)> callback);
    void setOnTimeUpdate(std::function<void(double)> callback);
    void setOnEnded(std::function<void()> callback);
    void setOnError(std::function<void(const std::string&)> callback);
    void setOnBuffering(std::function<void(double)> callback);
    
    // ==========================================================================
    // Frame Loop (call from main thread)
    // ==========================================================================
    
    /**
     * @brief Update playback and render frame
     * Call this from your render loop
     */
    void update();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Default Implementations
// =============================================================================

/**
 * @brief OpenGL renderer implementation
 */
class GLMediaRenderer : public MediaRenderer {
public:
    GLMediaRenderer();
    ~GLMediaRenderer() override;
    
    bool initialize();
    void shutdown();
    
    uint32_t uploadFrame(const VideoFrame& frame) override;
    void renderFrame(uint32_t textureId, int x, int y, int width, int height) override;
    void present() override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief NXAudio output implementation
 */
class NXAudioOutput : public AudioOutput {
public:
    NXAudioOutput();
    ~NXAudioOutput() override;
    
    bool initialize(int sampleRate, int channels) override;
    void queueSamples(const AudioSamples& samples) override;
    double getPlaybackPosition() const override;
    void pause(bool paused) override;
    void flush() override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Get global media pipeline instance
 */
MediaPipeline& getMediaPipeline();

} // namespace media
} // namespace zepra

#endif // ZEPRA_MEDIA_PIPELINE_H
