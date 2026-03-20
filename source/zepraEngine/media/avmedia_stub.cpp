// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file avmedia_stub.cpp
 * @brief Minimal stub implementations for VideoProcessor
 * @note Real implementation would use hardware decode APIs. This stub allows build.
 */

#include "engine/video_processor.h"

namespace zepra {
namespace video {

// Impl class for pimpl pattern
class VideoProcessor::Impl {
public:
    bool initialized = false;
    VideoConfig config;
    bool hardwareDecodeEnabled = false;
};

// =============================================================================
// VideoProcessor Stubs
// =============================================================================

VideoProcessor::VideoProcessor() : impl_(std::make_unique<Impl>()) {}
VideoProcessor::~VideoProcessor() = default;

bool VideoProcessor::initialize() { impl_->initialized = true; return true; }
void VideoProcessor::shutdown() { impl_->initialized = false; }
bool VideoProcessor::isInitialized() const { return impl_->initialized; }

VideoStreamInfo VideoProcessor::detectCodec(const uint8_t* data, size_t size) { return VideoStreamInfo(); }
VideoStreamInfo VideoProcessor::detectFromFile(const std::string& filepath) { return VideoStreamInfo(); }
std::string VideoProcessor::codecName(VideoCodec codec) { return "Unknown"; }

std::vector<HardwareCapabilities> VideoProcessor::getHardwareDecoders() const { return {}; }
bool VideoProcessor::isHardwareDecodeAvailable(VideoCodec codec) const { return false; }
HardwareDecoder VideoProcessor::getBestDecoder(const VideoStreamInfo& info) const { return HardwareDecoder::None; }
void VideoProcessor::setHardwareDecodeEnabled(bool enabled) { impl_->hardwareDecodeEnabled = enabled; }
bool VideoProcessor::isHardwareDecodeEnabled() const { return impl_->hardwareDecodeEnabled; }

void VideoProcessor::applyPreset(VideoPreset preset) { impl_->config.preset = preset; }
VideoPreset VideoProcessor::getCurrentPreset() const { return impl_->config.preset; }
std::string VideoProcessor::presetName(VideoPreset preset) {
    switch(preset) {
        case VideoPreset::Default: return "Default";
        case VideoPreset::Cinema: return "Cinema";
        case VideoPreset::Vivid: return "Vivid";
        case VideoPreset::Natural: return "Natural";
        case VideoPreset::Gaming: return "Gaming";
        case VideoPreset::Reading: return "Reading";
        case VideoPreset::Night: return "Night";
        case VideoPreset::HDRSimulation: return "HDR Simulation";
        case VideoPreset::Custom: return "Custom";
        default: return "Unknown";
    }
}
std::vector<VideoPreset> VideoProcessor::getAllPresets() {
    return { VideoPreset::Default, VideoPreset::Cinema, VideoPreset::Vivid, 
             VideoPreset::Natural, VideoPreset::Gaming };
}

void VideoProcessor::setBrightness(float value) { impl_->config.adjustments.brightness = value; }
float VideoProcessor::getBrightness() const { return impl_->config.adjustments.brightness; }
void VideoProcessor::setContrast(float value) { impl_->config.adjustments.contrast = value; }
float VideoProcessor::getContrast() const { return impl_->config.adjustments.contrast; }
void VideoProcessor::setSaturation(float value) { impl_->config.adjustments.saturation = value; }
float VideoProcessor::getSaturation() const { return impl_->config.adjustments.saturation; }
void VideoProcessor::setHue(float value) { impl_->config.adjustments.hue = value; }
float VideoProcessor::getHue() const { return impl_->config.adjustments.hue; }
void VideoProcessor::setGamma(float value) { impl_->config.adjustments.gamma = value; }
float VideoProcessor::getGamma() const { return impl_->config.adjustments.gamma; }
void VideoProcessor::setSharpness(float value) { impl_->config.adjustments.sharpness = value; }
float VideoProcessor::getSharpness() const { return impl_->config.adjustments.sharpness; }
void VideoProcessor::setAdjustments(const VideoAdjustments& adj) { impl_->config.adjustments = adj; }
VideoAdjustments VideoProcessor::getAdjustments() const { return impl_->config.adjustments; }
void VideoProcessor::resetAdjustments() { impl_->config.adjustments = VideoAdjustments(); }

void VideoProcessor::setAIEnhancement(const AIEnhancement& ai) { impl_->config.ai = ai; }
AIEnhancement VideoProcessor::getAIEnhancement() const { return impl_->config.ai; }
void VideoProcessor::setAIUpscaling(bool enabled, int targetWidth, int targetHeight) {
    impl_->config.ai.upscalingEnabled = enabled;
    impl_->config.ai.targetWidth = targetWidth;
    impl_->config.ai.targetHeight = targetHeight;
}
void VideoProcessor::setAIDenoise(bool enabled, float strength) {
    impl_->config.ai.denoiseEnabled = enabled;
    impl_->config.ai.denoiseStrength = strength;
}
void VideoProcessor::setFrameInterpolation(bool enabled, int targetFPS) {
    impl_->config.ai.frameInterpolation = enabled;
    impl_->config.ai.targetFPS = targetFPS;
}
bool VideoProcessor::isAIAvailable() const { return false; }

void VideoProcessor::setToneMapping(const ToneMappingSettings& settings) { impl_->config.toneMapping = settings; }
ToneMappingSettings VideoProcessor::getToneMapping() const { return impl_->config.toneMapping; }
void VideoProcessor::setToneMappingEnabled(bool enabled) { impl_->config.toneMapping.enabled = enabled; }

void VideoProcessor::setScaling(const ScalingSettings& settings) { impl_->config.scaling = settings; }
ScalingSettings VideoProcessor::getScaling() const { return impl_->config.scaling; }

void VideoProcessor::setAudioVideoOffset(float offsetMs) { impl_->config.audioVideoOffset = offsetMs; }
float VideoProcessor::getAudioVideoOffset() const { return impl_->config.audioVideoOffset; }

void VideoProcessor::setLowPowerMode(bool enabled) { impl_->config.lowPowerMode = enabled; }
bool VideoProcessor::isLowPowerMode() const { return impl_->config.lowPowerMode; }

void VideoProcessor::setConfig(const VideoConfig& config) { impl_->config = config; }
VideoConfig VideoProcessor::getConfig() const { return impl_->config; }
void VideoProcessor::resetToDefaults() { impl_->config = VideoConfig(); }

void VideoProcessor::processFrame(const uint8_t* input, uint8_t* output, int width, int height) {
    // Stub - just copy input to output
}

VideoProcessor::Stats VideoProcessor::getStats() const {
    return { 30.0f, 30.0f, 0.05f, 0.0f, false, "Software", 0 };
}

void VideoProcessor::setOnConfigChanged(std::function<void(const VideoConfig&)> callback) {}

VideoProcessor& getVideoProcessor() {
    static VideoProcessor instance;
    return instance;
}

} // namespace video
} // namespace zepra
