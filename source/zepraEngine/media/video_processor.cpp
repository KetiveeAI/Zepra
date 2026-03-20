// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file video_processor.cpp
 * @brief Video processing implementation
 * 
 * Hardware decode detection, video enhancement, and AI placeholders.
 */

#include "engine/video_processor.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace zepra {
namespace video {

// =============================================================================
// Codec Detection Constants
// =============================================================================

// H.264/AVC NAL unit start code
static const uint8_t H264_START_CODE[] = {0x00, 0x00, 0x00, 0x01};
static const uint8_t H264_NAL_SPS = 0x67;      // Sequence Parameter Set
static const uint8_t H264_NAL_PPS = 0x68;      // Picture Parameter Set

// H.265/HEVC
static const uint8_t HEVC_NAL_VPS = 0x40;      // Video Parameter Set
static const uint8_t HEVC_NAL_SPS = 0x42;

// VP8/VP9 signature
static const uint8_t VP8_SIGNATURE[] = {0x9D, 0x01, 0x2A};
static const uint8_t VP9_SIGNATURE[] = {0x49, 0x83, 0x42};

// AV1 OBU header
static const uint8_t AV1_OBU_SEQUENCE = 0x0A;

// Container magic bytes
static const uint8_t MP4_FTYP[] = {0x66, 0x74, 0x79, 0x70}; // "ftyp"
static const uint8_t WEBM_MAGIC[] = {0x1A, 0x45, 0xDF, 0xA3}; // EBML
static const uint8_t MKV_MAGIC[] = {0x1A, 0x45, 0xDF, 0xA3};

// =============================================================================
// Video Preset Configurations
// =============================================================================

struct PresetConfig {
    VideoAdjustments adjustments;
    bool useAI;
};

static const PresetConfig PRESET_CONFIGS[] = {
    // Default - no processing
    {{0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}, false},
    
    // Cinema - warm, filmic
    {{0.05f, 1.1f, 0.95f, 5.0f, 1.1f, 0.1f}, false},
    
    // Vivid - saturated
    {{0.0f, 1.15f, 1.4f, 0.0f, 1.0f, 0.15f}, false},
    
    // Natural - accurate
    {{0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}, false},
    
    // Gaming - low latency, sharp
    {{0.0f, 1.05f, 1.1f, 0.0f, 1.0f, 0.3f}, false},
    
    // Reading - warm, reduced blue
    {{0.1f, 0.95f, 0.9f, 10.0f, 1.1f, 0.0f}, false},
    
    // Night - reduced brightness
    {{-0.3f, 0.9f, 0.85f, 5.0f, 1.2f, 0.0f}, false},
    
    // HDR Simulation
    {{0.0f, 1.2f, 1.15f, 0.0f, 0.9f, 0.2f}, false},
    
    // Custom
    {{0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f}, false},
};

// =============================================================================
// Helper Functions
// =============================================================================

std::string VideoProcessor::codecName(VideoCodec codec) {
    switch (codec) {
        case VideoCodec::H264: return "H.264/AVC";
        case VideoCodec::H265: return "H.265/HEVC";
        case VideoCodec::VP8: return "VP8";
        case VideoCodec::VP9: return "VP9";
        case VideoCodec::AV1: return "AV1";
        case VideoCodec::MPEG4: return "MPEG-4";
        case VideoCodec::MPEG2: return "MPEG-2";
        case VideoCodec::ProRes: return "Apple ProRes";
        case VideoCodec::DNxHD: return "Avid DNxHD";
        default: return "Unknown";
    }
}

std::string VideoProcessor::presetName(VideoPreset preset) {
    switch (preset) {
        case VideoPreset::Default: return "Default";
        case VideoPreset::Cinema: return "Cinema";
        case VideoPreset::Vivid: return "Vivid";
        case VideoPreset::Natural: return "Natural";
        case VideoPreset::Gaming: return "Gaming";
        case VideoPreset::Reading: return "Reading";
        case VideoPreset::Night: return "Night Mode";
        case VideoPreset::HDRSimulation: return "HDR Simulation";
        case VideoPreset::Custom: return "Custom";
        default: return "Unknown";
    }
}

std::vector<VideoPreset> VideoProcessor::getAllPresets() {
    return {
        VideoPreset::Default,
        VideoPreset::Cinema,
        VideoPreset::Vivid,
        VideoPreset::Natural,
        VideoPreset::Gaming,
        VideoPreset::Reading,
        VideoPreset::Night,
        VideoPreset::HDRSimulation,
        VideoPreset::Custom
    };
}

// =============================================================================
// VideoProcessor Implementation
// =============================================================================

class VideoProcessor::Impl {
public:
    bool initialized = false;
    VideoConfig config;
    std::vector<HardwareCapabilities> hwDecoders;
    std::function<void(const VideoConfig&)> onConfigChanged;
    
    // Statistics
    Stats stats = {};
    
    void detectHardwareDecoders() {
        hwDecoders.clear();
        
        // Detect platform-specific hardware decoders
        #if defined(__linux__) && !defined(PLATFORM_NEOLYX)
            // VAAPI (Intel/AMD)
            HardwareCapabilities vaapi;
            vaapi.decoder = HardwareDecoder::VAAPI;
            vaapi.deviceName = "VAAPI";
            vaapi.supportsH264 = true;
            vaapi.supportsH265 = true;
            vaapi.supportsVP9 = true;
            vaapi.supportsAV1 = false;  // Depends on hardware
            vaapi.maxWidth = 4096;
            vaapi.maxHeight = 2160;
            vaapi.supports10bit = true;
            hwDecoders.push_back(vaapi);
            
            // NVDEC (NVIDIA)
            // TODO: Check if NVIDIA GPU present
            HardwareCapabilities nvdec;
            nvdec.decoder = HardwareDecoder::NVDEC;
            nvdec.deviceName = "NVIDIA NVDEC";
            nvdec.supportsH264 = true;
            nvdec.supportsH265 = true;
            nvdec.supportsVP9 = true;
            nvdec.supportsAV1 = true;  // RTX 30+
            nvdec.maxWidth = 8192;
            nvdec.maxHeight = 4320;
            nvdec.supports10bit = true;
            nvdec.supportsHDR = true;
            // hwDecoders.push_back(nvdec);  // Only if NVIDIA detected
        #endif
        
        #if defined(__APPLE__)
            HardwareCapabilities vt;
            vt.decoder = HardwareDecoder::VideoToolbox;
            vt.deviceName = "Apple VideoToolbox";
            vt.supportsH264 = true;
            vt.supportsH265 = true;
            vt.supportsVP9 = false;  // No VP9 on VideoToolbox
            vt.supportsAV1 = false;
            vt.maxWidth = 4096;
            vt.maxHeight = 2160;
            vt.supports10bit = true;
            vt.supportsHDR = true;
            hwDecoders.push_back(vt);
        #endif
        
        #if defined(_WIN32)
            HardwareCapabilities dxva;
            dxva.decoder = HardwareDecoder::D3D11VA;
            dxva.deviceName = "D3D11 Video Acceleration";
            dxva.supportsH264 = true;
            dxva.supportsH265 = true;
            dxva.supportsVP9 = true;
            dxva.supportsAV1 = true;
            dxva.maxWidth = 4096;
            dxva.maxHeight = 2160;
            dxva.supports10bit = true;
            hwDecoders.push_back(dxva);
        #endif
        
        #if defined(PLATFORM_NEOLYX)
            // NeolyxOS native decoder
            HardwareCapabilities neolyx;
            neolyx.decoder = HardwareDecoder::Vulkan;
            neolyx.deviceName = "NeolyxOS Hardware Decoder";
            neolyx.supportsH264 = true;
            neolyx.supportsH265 = true;
            neolyx.supportsVP9 = true;
            neolyx.supportsAV1 = true;
            neolyx.maxWidth = 8192;
            neolyx.maxHeight = 4320;
            neolyx.supports10bit = true;
            neolyx.supportsHDR = true;
            neolyx.supportsDeinterlace = true;
            neolyx.supportsScaling = true;
            hwDecoders.push_back(neolyx);
        #endif
    }
    
    void notifyChange() {
        if (onConfigChanged) {
            onConfigChanged(config);
        }
    }
    
    // Apply adjustments to a pixel
    void applyAdjustments(uint8_t& r, uint8_t& g, uint8_t& b) {
        const auto& adj = config.adjustments;
        
        // Convert to float [0, 1]
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;
        
        // Brightness
        rf += adj.brightness;
        gf += adj.brightness;
        bf += adj.brightness;
        
        // Contrast (around 0.5)
        rf = (rf - 0.5f) * adj.contrast + 0.5f;
        gf = (gf - 0.5f) * adj.contrast + 0.5f;
        bf = (bf - 0.5f) * adj.contrast + 0.5f;
        
        // Saturation
        float gray = 0.299f * rf + 0.587f * gf + 0.114f * bf;
        rf = gray + (rf - gray) * adj.saturation;
        gf = gray + (gf - gray) * adj.saturation;
        bf = gray + (bf - gray) * adj.saturation;
        
        // Gamma
        rf = std::pow(rf, 1.0f / adj.gamma);
        gf = std::pow(gf, 1.0f / adj.gamma);
        bf = std::pow(bf, 1.0f / adj.gamma);
        
        // Clamp and convert back
        r = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, rf)) * 255.0f);
        g = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, gf)) * 255.0f);
        b = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, bf)) * 255.0f);
    }
};

VideoProcessor::VideoProcessor() : impl_(std::make_unique<Impl>()) {}
VideoProcessor::~VideoProcessor() = default;

bool VideoProcessor::initialize() {
    if (impl_->initialized) return true;
    
    impl_->detectHardwareDecoders();
    impl_->initialized = true;
    
    return true;
}

void VideoProcessor::shutdown() {
    impl_->initialized = false;
}

bool VideoProcessor::isInitialized() const {
    return impl_->initialized;
}

// =============================================================================
// Codec Detection
// =============================================================================

VideoStreamInfo VideoProcessor::detectCodec(const uint8_t* data, size_t size) {
    VideoStreamInfo info;
    
    if (!data || size < 8) return info;
    
    // Check for container first
    if (size >= 8 && std::memcmp(data + 4, MP4_FTYP, 4) == 0) {
        info.container = VideoContainer::MP4;
        // MP4 usually contains H.264 or H.265
        info.codec = VideoCodec::H264;  // Default assumption
    }
    
    if (std::memcmp(data, WEBM_MAGIC, 4) == 0) {
        info.container = VideoContainer::WebM;
        info.codec = VideoCodec::VP9;  // WebM usually VP9 or VP8
    }
    
    // Check for H.264 NAL units
    for (size_t i = 0; i < size - 5; ++i) {
        if (std::memcmp(data + i, H264_START_CODE, 4) == 0) {
            uint8_t nalType = data[i + 4] & 0x1F;
            if (nalType == 7) {  // SPS
                info.codec = VideoCodec::H264;
                break;
            }
        }
    }
    
    // Check for H.265 NAL units
    for (size_t i = 0; i < size - 5; ++i) {
        if (std::memcmp(data + i, H264_START_CODE, 4) == 0) {
            uint8_t nalType = (data[i + 4] >> 1) & 0x3F;
            if (nalType == 32 || nalType == 33) {  // VPS or SPS
                info.codec = VideoCodec::H265;
                break;
            }
        }
    }
    
    return info;
}

VideoStreamInfo VideoProcessor::detectFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return VideoStreamInfo();
    
    uint8_t header[256];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    return detectCodec(header, file.gcount());
}

// =============================================================================
// Hardware Decode
// =============================================================================

std::vector<HardwareCapabilities> VideoProcessor::getHardwareDecoders() const {
    return impl_->hwDecoders;
}

bool VideoProcessor::isHardwareDecodeAvailable(VideoCodec codec) const {
    for (const auto& hw : impl_->hwDecoders) {
        switch (codec) {
            case VideoCodec::H264: if (hw.supportsH264) return true; break;
            case VideoCodec::H265: if (hw.supportsH265) return true; break;
            case VideoCodec::VP9: if (hw.supportsVP9) return true; break;
            case VideoCodec::AV1: if (hw.supportsAV1) return true; break;
            default: break;
        }
    }
    return false;
}

HardwareDecoder VideoProcessor::getBestDecoder(const VideoStreamInfo& info) const {
    for (const auto& hw : impl_->hwDecoders) {
        bool supports = false;
        switch (info.codec) {
            case VideoCodec::H264: supports = hw.supportsH264; break;
            case VideoCodec::H265: supports = hw.supportsH265; break;
            case VideoCodec::VP9: supports = hw.supportsVP9; break;
            case VideoCodec::AV1: supports = hw.supportsAV1; break;
            default: break;
        }
        
        if (supports && info.width <= hw.maxWidth && info.height <= hw.maxHeight) {
            return hw.decoder;
        }
    }
    return HardwareDecoder::None;
}

void VideoProcessor::setHardwareDecodeEnabled(bool enabled) {
    impl_->config.preferHardwareDecode = enabled;
    impl_->notifyChange();
}

bool VideoProcessor::isHardwareDecodeEnabled() const {
    return impl_->config.preferHardwareDecode;
}

// =============================================================================
// Presets
// =============================================================================

void VideoProcessor::applyPreset(VideoPreset preset) {
    int idx = static_cast<int>(preset);
    if (idx >= 0 && idx < 9) {
        impl_->config.preset = preset;
        impl_->config.adjustments = PRESET_CONFIGS[idx].adjustments;
        impl_->notifyChange();
    }
}

VideoPreset VideoProcessor::getCurrentPreset() const {
    return impl_->config.preset;
}

// =============================================================================
// Adjustments
// =============================================================================

void VideoProcessor::setBrightness(float value) {
    impl_->config.adjustments.brightness = std::max(-1.0f, std::min(1.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getBrightness() const {
    return impl_->config.adjustments.brightness;
}

void VideoProcessor::setContrast(float value) {
    impl_->config.adjustments.contrast = std::max(0.0f, std::min(2.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getContrast() const {
    return impl_->config.adjustments.contrast;
}

void VideoProcessor::setSaturation(float value) {
    impl_->config.adjustments.saturation = std::max(0.0f, std::min(2.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getSaturation() const {
    return impl_->config.adjustments.saturation;
}

void VideoProcessor::setHue(float value) {
    impl_->config.adjustments.hue = std::max(-180.0f, std::min(180.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getHue() const {
    return impl_->config.adjustments.hue;
}

void VideoProcessor::setGamma(float value) {
    impl_->config.adjustments.gamma = std::max(0.1f, std::min(4.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getGamma() const {
    return impl_->config.adjustments.gamma;
}

void VideoProcessor::setSharpness(float value) {
    impl_->config.adjustments.sharpness = std::max(0.0f, std::min(1.0f, value));
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

float VideoProcessor::getSharpness() const {
    return impl_->config.adjustments.sharpness;
}

void VideoProcessor::setAdjustments(const VideoAdjustments& adj) {
    impl_->config.adjustments = adj;
    impl_->config.preset = VideoPreset::Custom;
    impl_->notifyChange();
}

VideoAdjustments VideoProcessor::getAdjustments() const {
    return impl_->config.adjustments;
}

void VideoProcessor::resetAdjustments() {
    impl_->config.adjustments = VideoAdjustments();
    impl_->config.preset = VideoPreset::Default;
    impl_->notifyChange();
}

// =============================================================================
// AI Enhancement
// =============================================================================

void VideoProcessor::setAIEnhancement(const AIEnhancement& ai) {
    impl_->config.ai = ai;
    impl_->notifyChange();
}

AIEnhancement VideoProcessor::getAIEnhancement() const {
    return impl_->config.ai;
}

void VideoProcessor::setAIUpscaling(bool enabled, int targetWidth, int targetHeight) {
    impl_->config.ai.upscalingEnabled = enabled;
    impl_->config.ai.enabled = enabled;
    if (targetWidth > 0) impl_->config.ai.targetWidth = targetWidth;
    if (targetHeight > 0) impl_->config.ai.targetHeight = targetHeight;
    impl_->notifyChange();
}

void VideoProcessor::setAIDenoise(bool enabled, float strength) {
    impl_->config.ai.denoiseEnabled = enabled;
    impl_->config.ai.denoiseStrength = strength;
    impl_->config.ai.enabled = enabled || impl_->config.ai.upscalingEnabled;
    impl_->notifyChange();
}

void VideoProcessor::setFrameInterpolation(bool enabled, int targetFPS) {
    impl_->config.ai.frameInterpolation = enabled;
    impl_->config.ai.targetFPS = targetFPS;
    impl_->notifyChange();
}

bool VideoProcessor::isAIAvailable() const {
    // AI requires GPU acceleration
    return !impl_->hwDecoders.empty();
}

// =============================================================================
// Tone Mapping
// =============================================================================

void VideoProcessor::setToneMapping(const ToneMappingSettings& settings) {
    impl_->config.toneMapping = settings;
    impl_->notifyChange();
}

ToneMappingSettings VideoProcessor::getToneMapping() const {
    return impl_->config.toneMapping;
}

void VideoProcessor::setToneMappingEnabled(bool enabled) {
    impl_->config.toneMapping.enabled = enabled;
    impl_->notifyChange();
}

// =============================================================================
// Scaling
// =============================================================================

void VideoProcessor::setScaling(const ScalingSettings& settings) {
    impl_->config.scaling = settings;
    impl_->notifyChange();
}

ScalingSettings VideoProcessor::getScaling() const {
    return impl_->config.scaling;
}

// =============================================================================
// Audio/Video Sync
// =============================================================================

void VideoProcessor::setAudioVideoOffset(float offsetMs) {
    impl_->config.audioVideoOffset = offsetMs;
    impl_->notifyChange();
}

float VideoProcessor::getAudioVideoOffset() const {
    return impl_->config.audioVideoOffset;
}

// =============================================================================
// Power Mode
// =============================================================================

void VideoProcessor::setLowPowerMode(bool enabled) {
    impl_->config.lowPowerMode = enabled;
    impl_->notifyChange();
}

bool VideoProcessor::isLowPowerMode() const {
    return impl_->config.lowPowerMode;
}

// =============================================================================
// Full Config
// =============================================================================

void VideoProcessor::setConfig(const VideoConfig& config) {
    impl_->config = config;
    impl_->notifyChange();
}

VideoConfig VideoProcessor::getConfig() const {
    return impl_->config;
}

void VideoProcessor::resetToDefaults() {
    impl_->config = VideoConfig();
    impl_->notifyChange();
}

// =============================================================================
// Processing
// =============================================================================

void VideoProcessor::processFrame(const uint8_t* input, uint8_t* output,
                                   int width, int height) {
    if (!input || !output) return;
    
    // Copy and apply adjustments
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            
            uint8_t r = input[idx];
            uint8_t g = input[idx + 1];
            uint8_t b = input[idx + 2];
            
            impl_->applyAdjustments(r, g, b);
            
            output[idx] = r;
            output[idx + 1] = g;
            output[idx + 2] = b;
        }
    }
}

VideoProcessor::Stats VideoProcessor::getStats() const {
    return impl_->stats;
}

void VideoProcessor::setOnConfigChanged(
    std::function<void(const VideoConfig&)> callback) {
    impl_->onConfigChanged = callback;
}

// =============================================================================
// Global Instance
// =============================================================================

VideoProcessor& getVideoProcessor() {
    static VideoProcessor instance;
    return instance;
}

} // namespace video
} // namespace zepra
