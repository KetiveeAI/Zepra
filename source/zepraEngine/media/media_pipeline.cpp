// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file media_pipeline.cpp
 * @brief Unified media pipeline implementation
 * 
 * Connects video decoding to OpenGL rendering and
 * audio decoding to NXAudio output.
 */

#include "engine/media_pipeline.h"
#include "engine/video_processor.h"
#include "engine/audio_equalizer.h"
#include "engine/browser_audio.h"

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <cstring>

// NXAudio C interface
extern "C" {
#include "nxaudio/nxaudio.h"
}

namespace zepra {
namespace media {

// =============================================================================
// MediaPipeline Implementation
// =============================================================================

class MediaPipeline::Impl {
public:
    MediaRenderer* renderer = nullptr;
    AudioOutput* audioOutput = nullptr;
    
    MediaInfo mediaInfo;
    PlaybackState state = PlaybackState::Stopped;
    
    std::atomic<bool> loaded{false};
    std::atomic<double> currentTime{0.0};
    std::atomic<double> duration{0.0};
    std::atomic<float> volume{1.0f};
    std::atomic<bool> muted{false};
    std::atomic<float> playbackRate{1.0f};
    
    bool hwDecodeEnabled = true;
    bool audioProcessingEnabled = true;
    bool videoProcessingEnabled = true;
    
    // Current frame data
    std::vector<uint8_t> frameBuffer;
    uint32_t currentTextureId = 0;
    int frameWidth = 0;
    int frameHeight = 0;
    
    // Callbacks
    std::function<void(PlaybackState)> onStateChange;
    std::function<void(double)> onTimeUpdate;
    std::function<void()> onEnded;
    std::function<void(const std::string&)> onError;
    std::function<void(double)> onBuffering;
    
    // Audio buffer for processing
    std::vector<float> audioBuffer;
    
    void setState(PlaybackState newState) {
        state = newState;
        if (onStateChange) onStateChange(newState);
    }
    
    void updateTime(double time) {
        currentTime = time;
        if (onTimeUpdate) onTimeUpdate(time);
    }
};

MediaPipeline::MediaPipeline() : impl_(std::make_unique<Impl>()) {}
MediaPipeline::~MediaPipeline() { stop(); }

void MediaPipeline::setRenderer(MediaRenderer* renderer) {
    impl_->renderer = renderer;
}

void MediaPipeline::setAudioOutput(AudioOutput* output) {
    impl_->audioOutput = output;
}

bool MediaPipeline::load(const std::string& url) {
    std::cout << "[MediaPipeline] Loading: " << url << std::endl;
    
    // Parse URL/file extension for format detection
    impl_->mediaInfo = MediaInfo();
    
    // Detect video format from extension
    if (url.find(".mp4") != std::string::npos || 
        url.find(".webm") != std::string::npos ||
        url.find(".mkv") != std::string::npos) {
        impl_->mediaInfo.hasVideo = true;
        impl_->mediaInfo.hasAudio = true;
    }
    
    // TODO: Real decoding with FFmpeg/GStreamer
    // For now, simulate loading success
    impl_->mediaInfo.videoWidth = 1920;
    impl_->mediaInfo.videoHeight = 1080;
    impl_->mediaInfo.videoFPS = 30.0f;
    impl_->mediaInfo.videoCodec = "H.264";
    impl_->mediaInfo.audioSampleRate = 48000;
    impl_->mediaInfo.audioChannels = 2;
    impl_->mediaInfo.audioCodec = "AAC";
    impl_->mediaInfo.duration = 120.0;  // 2 minutes placeholder
    
    impl_->duration = impl_->mediaInfo.duration;
    impl_->loaded = true;
    impl_->setState(PlaybackState::Paused);
    
    // Initialize audio output if available
    if (impl_->audioOutput) {
        impl_->audioOutput->initialize(
            impl_->mediaInfo.audioSampleRate,
            impl_->mediaInfo.audioChannels);
    }
    
    return true;
}

bool MediaPipeline::loadFromMemory(const uint8_t* data, size_t size, 
                                    const std::string& mimeType) {
    std::cout << "[MediaPipeline] Loading from memory (" << size 
              << " bytes, " << mimeType << ")" << std::endl;
    
    // Detect container from MIME type
    if (mimeType.find("video/mp4") != std::string::npos) {
        impl_->mediaInfo.container = "MP4";
    } else if (mimeType.find("video/webm") != std::string::npos) {
        impl_->mediaInfo.container = "WebM";
    }
    
    impl_->loaded = true;
    return true;
}

MediaInfo MediaPipeline::getMediaInfo() const {
    return impl_->mediaInfo;
}

bool MediaPipeline::isLoaded() const {
    return impl_->loaded;
}

void MediaPipeline::play() {
    if (!impl_->loaded) return;
    
    std::cout << "[MediaPipeline] Play" << std::endl;
    impl_->setState(PlaybackState::Playing);
    
    if (impl_->audioOutput) {
        impl_->audioOutput->pause(false);
    }
}

void MediaPipeline::pause() {
    if (!impl_->loaded) return;
    
    std::cout << "[MediaPipeline] Pause" << std::endl;
    impl_->setState(PlaybackState::Paused);
    
    if (impl_->audioOutput) {
        impl_->audioOutput->pause(true);
    }
}

void MediaPipeline::stop() {
    std::cout << "[MediaPipeline] Stop" << std::endl;
    impl_->setState(PlaybackState::Stopped);
    impl_->currentTime = 0.0;
    
    if (impl_->audioOutput) {
        impl_->audioOutput->flush();
    }
}

void MediaPipeline::togglePlayPause() {
    if (impl_->state == PlaybackState::Playing) {
        pause();
    } else {
        play();
    }
}

void MediaPipeline::seek(double seconds) {
    if (!impl_->loaded) return;
    
    seconds = std::max(0.0, std::min(seconds, impl_->duration.load()));
    
    std::cout << "[MediaPipeline] Seek to " << seconds << "s" << std::endl;
    impl_->currentTime = seconds;
    
    if (impl_->audioOutput) {
        impl_->audioOutput->flush();
    }
    
    impl_->updateTime(seconds);
}

PlaybackState MediaPipeline::getState() const {
    return impl_->state;
}

double MediaPipeline::getCurrentTime() const {
    return impl_->currentTime;
}

double MediaPipeline::getDuration() const {
    return impl_->duration;
}

double MediaPipeline::getBufferedTime() const {
    // TODO: Real buffer tracking
    return impl_->duration;
}

bool MediaPipeline::isPlaying() const {
    return impl_->state == PlaybackState::Playing;
}

bool MediaPipeline::isPaused() const {
    return impl_->state == PlaybackState::Paused;
}

bool MediaPipeline::isSeeking() const {
    return false;  // TODO
}

void MediaPipeline::setVolume(float volume) {
    impl_->volume = std::max(0.0f, std::min(1.0f, volume));
    
    // Apply to audio system
    auto& audio = zepra::audio::getBrowserAudio();
    audio.setMasterVolume(impl_->volume);
}

float MediaPipeline::getVolume() const {
    return impl_->volume;
}

void MediaPipeline::setMuted(bool muted) {
    impl_->muted = muted;
    
    auto& audio = zepra::audio::getBrowserAudio();
    audio.setMuted(muted);
}

bool MediaPipeline::isMuted() const {
    return impl_->muted;
}

VideoFrame MediaPipeline::getCurrentFrame() {
    VideoFrame frame;
    frame.width = impl_->frameWidth;
    frame.height = impl_->frameHeight;
    frame.data = impl_->frameBuffer.data();
    frame.stride = impl_->frameWidth * 3;
    frame.pts = impl_->currentTime;
    frame.textureId = impl_->currentTextureId;
    return frame;
}

void MediaPipeline::setPlaybackRate(float rate) {
    impl_->playbackRate = std::max(0.25f, std::min(4.0f, rate));
    
    // Apply to audio pitch control
    auto& eq = zepra::audio::getAudioEqualizer();
    eq.setSpeed(impl_->playbackRate);
}

float MediaPipeline::getPlaybackRate() const {
    return impl_->playbackRate;
}

void MediaPipeline::setHardwareDecodeEnabled(bool enabled) {
    impl_->hwDecodeEnabled = enabled;
    
    auto& video = zepra::video::getVideoProcessor();
    video.setHardwareDecodeEnabled(enabled);
}

bool MediaPipeline::isHardwareDecodeEnabled() const {
    return impl_->hwDecodeEnabled;
}

void MediaPipeline::setAudioProcessingEnabled(bool enabled) {
    impl_->audioProcessingEnabled = enabled;
}

void MediaPipeline::setVideoProcessingEnabled(bool enabled) {
    impl_->videoProcessingEnabled = enabled;
}

void MediaPipeline::setOnStateChange(std::function<void(PlaybackState)> callback) {
    impl_->onStateChange = callback;
}

void MediaPipeline::setOnTimeUpdate(std::function<void(double)> callback) {
    impl_->onTimeUpdate = callback;
}

void MediaPipeline::setOnEnded(std::function<void()> callback) {
    impl_->onEnded = callback;
}

void MediaPipeline::setOnError(std::function<void(const std::string&)> callback) {
    impl_->onError = callback;
}

void MediaPipeline::setOnBuffering(std::function<void(double)> callback) {
    impl_->onBuffering = callback;
}

void MediaPipeline::update() {
    if (impl_->state != PlaybackState::Playing) return;
    
    // Advance playback time (simulate frame advance)
    double dt = 1.0 / 60.0 * impl_->playbackRate;  // Assuming 60 FPS update
    impl_->currentTime = impl_->currentTime + dt;
    
    // Check for end
    if (impl_->currentTime >= impl_->duration) {
        impl_->currentTime = impl_->duration.load();
        impl_->setState(PlaybackState::Stopped);
        if (impl_->onEnded) impl_->onEnded();
        return;
    }
    
    // TODO: Decode next video frame
    // For now, generate a test pattern
    int w = impl_->mediaInfo.videoWidth;
    int h = impl_->mediaInfo.videoHeight;
    
    if (impl_->frameBuffer.size() != w * h * 3) {
        impl_->frameBuffer.resize(w * h * 3);
        impl_->frameWidth = w;
        impl_->frameHeight = h;
    }
    
    // Simple animated test pattern
    int frameNum = static_cast<int>(impl_->currentTime * 30);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 3;
            impl_->frameBuffer[idx] = (x + frameNum) % 256;      // R
            impl_->frameBuffer[idx + 1] = (y + frameNum) % 256;  // G
            impl_->frameBuffer[idx + 2] = 128;                    // B
        }
    }
    
    // Apply video processing if enabled
    if (impl_->videoProcessingEnabled) {
        auto& video = zepra::video::getVideoProcessor();
        std::vector<uint8_t> processed(impl_->frameBuffer.size());
        video.processFrame(impl_->frameBuffer.data(), processed.data(), w, h);
        impl_->frameBuffer = std::move(processed);
    }
    
    // Upload to renderer
    if (impl_->renderer) {
        VideoFrame frame = getCurrentFrame();
        impl_->currentTextureId = impl_->renderer->uploadFrame(frame);
        
        // Render at full size (in real usage, position would come from layout)
        impl_->renderer->renderFrame(impl_->currentTextureId, 0, 0, w, h);
    }
    
    // Update time callback
    impl_->updateTime(impl_->currentTime);
}

// =============================================================================
// GLMediaRenderer Implementation
// =============================================================================

class GLMediaRenderer::Impl {
public:
    uint32_t textureId = 0;
    int texWidth = 0;
    int texHeight = 0;
    bool initialized = false;
    
    // Shader program
    uint32_t shaderProgram = 0;
    uint32_t vao = 0;
    uint32_t vbo = 0;
};

GLMediaRenderer::GLMediaRenderer() : impl_(std::make_unique<Impl>()) {}
GLMediaRenderer::~GLMediaRenderer() { shutdown(); }

bool GLMediaRenderer::initialize() {
    // Create texture
    glGenTextures(1, &impl_->textureId);
    glBindTexture(GL_TEXTURE_2D, impl_->textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    impl_->initialized = true;
    return true;
}

void GLMediaRenderer::shutdown() {
    if (impl_->textureId) {
        glDeleteTextures(1, &impl_->textureId);
        impl_->textureId = 0;
    }
    impl_->initialized = false;
}

uint32_t GLMediaRenderer::uploadFrame(const VideoFrame& frame) {
    if (!impl_->initialized || !frame.data) return 0;
    
    glBindTexture(GL_TEXTURE_2D, impl_->textureId);
    
    // Reallocate if size changed
    if (frame.width != impl_->texWidth || frame.height != impl_->texHeight) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                     frame.width, frame.height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, frame.data);
        impl_->texWidth = frame.width;
        impl_->texHeight = frame.height;
    } else {
        // Just update data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        frame.width, frame.height,
                        GL_RGB, GL_UNSIGNED_BYTE, frame.data);
    }
    
    return impl_->textureId;
}

void GLMediaRenderer::renderFrame(uint32_t textureId, int x, int y, int width, int height) {
    if (!impl_->initialized) return;
    
    // Simple fixed-function rendering for now
    // In production, use shaders from GLRenderBackend
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

void GLMediaRenderer::present() {
    // Swap handled by main loop
}

// =============================================================================
// NXAudioOutput Implementation
// =============================================================================

class NXAudioOutput::Impl {
public:
    bool initialized = false;
    int sampleRate = 48000;
    int channels = 2;
    bool paused = false;
    double playbackPos = 0.0;
    
    // Audio buffer queue
    std::vector<float> buffer;
    std::mutex bufferMutex;
    
    // NXAudio buffer
    nxaudio_buffer_t audioBuffer = NXAUDIO_INVALID_HANDLE;
};

NXAudioOutput::NXAudioOutput() : impl_(std::make_unique<Impl>()) {}
NXAudioOutput::~NXAudioOutput() {
    if (impl_->initialized) {
        // Cleanup NXAudio resources
    }
}

bool NXAudioOutput::initialize(int sampleRate, int channels) {
    impl_->sampleRate = sampleRate;
    impl_->channels = channels;
    
    // Initialize NXAudio if not already done
    auto& audio = zepra::audio::getBrowserAudio();
    if (!audio.isInitialized()) {
        audio.initialize();
    }
    
    impl_->initialized = true;
    return true;
}

void NXAudioOutput::queueSamples(const AudioSamples& samples) {
    if (!impl_->initialized) return;
    
    std::lock_guard<std::mutex> lock(impl_->bufferMutex);
    
    // Apply audio processing (equalizer, etc.)
    auto& eq = zepra::audio::getAudioEqualizer();
    
    std::vector<float> processed(samples.frameCount * 2);
    eq.process(samples.data, processed.data(), samples.frameCount);
    
    // Add to buffer
    impl_->buffer.insert(impl_->buffer.end(), 
                         processed.begin(), processed.end());
    
    // Update playback position
    impl_->playbackPos = samples.pts;
}

double NXAudioOutput::getPlaybackPosition() const {
    return impl_->playbackPos;
}

void NXAudioOutput::pause(bool paused) {
    impl_->paused = paused;
}

void NXAudioOutput::flush() {
    std::lock_guard<std::mutex> lock(impl_->bufferMutex);
    impl_->buffer.clear();
}

// =============================================================================
// Global Instance
// =============================================================================

MediaPipeline& getMediaPipeline() {
    static MediaPipeline instance;
    return instance;
}

} // namespace media
} // namespace zepra
