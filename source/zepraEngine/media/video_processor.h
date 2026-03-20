// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_VIDEO_PROCESSOR_H
#define ZEPRA_VIDEO_PROCESSOR_H

/**
 * @file video_processor.h
 * @brief Video processing and enhancement for browser playback
 * 
 * Features:
 * - Video codec detection (H.264, H.265, VP9, AV1)
 * - Hardware decode (VAAPI, NVDEC, VideoToolbox)
 * - Video enhancement (contrast, brightness, saturation)
 * - AI upscaling placeholders
 * - Audio/video synchronization
 * - HDR tone mapping
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace zepra {
namespace video {

// =============================================================================
// Video Codec Detection
// =============================================================================

/**
 * @brief Video codec types
 */
enum class VideoCodec {
    Unknown,
    
    // Common web codecs
    H264,           // AVC
    H265,           // HEVC
    VP8,
    VP9,
    AV1,
    
    // Legacy
    MPEG4,
    MPEG2,
    
    // Professional
    ProRes,
    DNxHD,
};

/**
 * @brief Video container formats
 */
enum class VideoContainer {
    Unknown,
    MP4,
    WebM,
    MKV,
    AVI,
    MOV,
    FLV,
    TS,
    HLS,
    DASH,
};

/**
 * @brief HDR format types
 */
enum class HDRFormat {
    SDR,            // Standard dynamic range
    HDR10,          // Static metadata
    HDR10Plus,      // Dynamic metadata
    DolbyVision,    // Dolby Vision
    HLG,            // Hybrid Log-Gamma
};

/**
 * @brief Video stream properties
 */
struct VideoStreamInfo {
    VideoCodec codec = VideoCodec::Unknown;
    VideoContainer container = VideoContainer::Unknown;
    
    // Resolution
    int width = 0;
    int height = 0;
    
    // Frame rate
    float fps = 0.0f;
    bool isVariableFPS = false;
    
    // Bitrate
    uint32_t bitrate = 0;
    
    // Color
    int bitDepth = 8;       // 8, 10, or 12 bit
    HDRFormat hdr = HDRFormat::SDR;
    std::string colorSpace;  // BT.709, BT.2020, etc.
    
    // Duration
    double durationSeconds = 0.0;
    
    // Codec details
    std::string profile;     // Main, High, etc.
    std::string level;       // 4.0, 5.1, etc.
};

// =============================================================================
// Hardware Decode
// =============================================================================

/**
 * @brief Hardware decode APIs
 */
enum class HardwareDecoder {
    None,           // Software decode
    
    // Linux
    VAAPI,          // Video Acceleration API (Intel/AMD)
    VDPAU,          // NVIDIA legacy
    NVDEC,          // NVIDIA modern
    V4L2,           // ARM devices
    
    // macOS
    VideoToolbox,
    
    // Windows
    DXVA2,
    D3D11VA,
    
    // Cross-platform
    Vulkan,         // Vulkan Video
};

/**
 * @brief Hardware decoder capabilities
 */
struct HardwareCapabilities {
    HardwareDecoder decoder = HardwareDecoder::None;
    std::string deviceName;
    
    // Codec support
    bool supportsH264 = false;
    bool supportsH265 = false;
    bool supportsVP9 = false;
    bool supportsAV1 = false;
    
    // Resolution limits
    int maxWidth = 0;
    int maxHeight = 0;
    
    // Features
    bool supports10bit = false;
    bool supportsHDR = false;
    bool supportsDeinterlace = false;
    bool supportsScaling = false;
};

// =============================================================================
// Video Enhancement Settings
// =============================================================================

/**
 * @brief Basic video adjustments
 */
struct VideoAdjustments {
    float brightness = 0.0f;    // -1.0 to 1.0
    float contrast = 1.0f;      // 0.0 to 2.0
    float saturation = 1.0f;    // 0.0 to 2.0
    float hue = 0.0f;           // -180 to 180 degrees
    float gamma = 1.0f;         // 0.1 to 4.0
    float sharpness = 0.0f;     // 0.0 to 1.0
};

/**
 * @brief AI enhancement options
 */
struct AIEnhancement {
    bool enabled = false;
    
    // Upscaling
    bool upscalingEnabled = false;
    int targetWidth = 0;
    int targetHeight = 0;
    
    // Noise reduction
    bool denoiseEnabled = false;
    float denoiseStrength = 0.5f;
    
    // Frame interpolation
    bool frameInterpolation = false;
    int targetFPS = 60;
    
    // Face enhancement
    bool faceEnhance = false;
    
    // HDR enhancement
    bool hdrEnhance = false;
};

/**
 * @brief Tone mapping settings (for HDR content)
 */
struct ToneMappingSettings {
    bool enabled = false;
    
    // HDR to SDR
    float peakLuminance = 1000.0f;  // nits
    float targetLuminance = 100.0f;
    
    // Algorithm
    enum Algorithm {
        Reinhard,
        Hable,
        ACES,
        BT2390
    } algorithm = BT2390;
    
    float exposure = 0.0f;
};

/**
 * @brief Video scaling settings
 */
struct ScalingSettings {
    enum Algorithm {
        Bilinear,
        Bicubic,
        Lanczos,
        Spline,
        NearestNeighbor,  // For pixel art
        AIUpscale         // AI-based
    } algorithm = Lanczos;
    
    bool maintainAspectRatio = true;
    bool allowUpscale = true;
};

/**
 * @brief One-click video presets
 */
enum class VideoPreset {
    Default,        // No processing
    Cinema,         // Movie-like colors
    Vivid,          // Saturated colors
    Natural,        // Accurate colors
    Gaming,         // Low latency
    Reading,        // Blue light filter
    Night,          // Reduced brightness
    HDRSimulation,  // Simulate HDR on SDR
    Custom,
};

// =============================================================================
// Full Video Configuration
// =============================================================================

/**
 * @brief Complete video processing configuration
 */
struct VideoConfig {
    // Preset
    VideoPreset preset = VideoPreset::Default;
    
    // Basic adjustments
    VideoAdjustments adjustments;
    
    // AI
    AIEnhancement ai;
    
    // Tone mapping
    ToneMappingSettings toneMapping;
    
    // Scaling
    ScalingSettings scaling;
    
    // Hardware
    bool preferHardwareDecode = true;
    HardwareDecoder preferredDecoder = HardwareDecoder::None;
    
    // Sync
    float audioVideoOffset = 0.0f;  // ms
    
    // Power
    bool lowPowerMode = false;
};

// =============================================================================
// Video Processor Class
// =============================================================================

/**
 * @brief Main video processor
 */
class VideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor();
    
    // ==========================================================================
    // Initialization
    // ==========================================================================
    
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // ==========================================================================
    // Codec Detection
    // ==========================================================================
    
    /**
     * @brief Detect video codec from data
     */
    VideoStreamInfo detectCodec(const uint8_t* data, size_t size);
    
    /**
     * @brief Detect from file
     */
    VideoStreamInfo detectFromFile(const std::string& filepath);
    
    /**
     * @brief Get codec name
     */
    static std::string codecName(VideoCodec codec);
    
    // ==========================================================================
    // Hardware Decode
    // ==========================================================================
    
    /**
     * @brief Get available hardware decoders
     */
    std::vector<HardwareCapabilities> getHardwareDecoders() const;
    
    /**
     * @brief Check if hardware decode available for codec
     */
    bool isHardwareDecodeAvailable(VideoCodec codec) const;
    
    /**
     * @brief Get best hardware decoder for video
     */
    HardwareDecoder getBestDecoder(const VideoStreamInfo& info) const;
    
    /**
     * @brief Enable/disable hardware decode
     */
    void setHardwareDecodeEnabled(bool enabled);
    bool isHardwareDecodeEnabled() const;
    
    // ==========================================================================
    // Video Presets
    // ==========================================================================
    
    /**
     * @brief Apply one-click preset
     */
    void applyPreset(VideoPreset preset);
    VideoPreset getCurrentPreset() const;
    
    /**
     * @brief Get preset name
     */
    static std::string presetName(VideoPreset preset);
    
    /**
     * @brief Get all presets
     */
    static std::vector<VideoPreset> getAllPresets();
    
    // ==========================================================================
    // Video Adjustments
    // ==========================================================================
    
    void setBrightness(float value);
    float getBrightness() const;
    
    void setContrast(float value);
    float getContrast() const;
    
    void setSaturation(float value);
    float getSaturation() const;
    
    void setHue(float value);
    float getHue() const;
    
    void setGamma(float value);
    float getGamma() const;
    
    void setSharpness(float value);
    float getSharpness() const;
    
    void setAdjustments(const VideoAdjustments& adj);
    VideoAdjustments getAdjustments() const;
    void resetAdjustments();
    
    // ==========================================================================
    // AI Enhancement
    // ==========================================================================
    
    void setAIEnhancement(const AIEnhancement& ai);
    AIEnhancement getAIEnhancement() const;
    
    void setAIUpscaling(bool enabled, int targetWidth = 0, int targetHeight = 0);
    void setAIDenoise(bool enabled, float strength = 0.5f);
    void setFrameInterpolation(bool enabled, int targetFPS = 60);
    
    bool isAIAvailable() const;
    
    // ==========================================================================
    // Tone Mapping
    // ==========================================================================
    
    void setToneMapping(const ToneMappingSettings& settings);
    ToneMappingSettings getToneMapping() const;
    void setToneMappingEnabled(bool enabled);
    
    // ==========================================================================
    // Scaling
    // ==========================================================================
    
    void setScaling(const ScalingSettings& settings);
    ScalingSettings getScaling() const;
    
    // ==========================================================================
    // Audio/Video Sync
    // ==========================================================================
    
    void setAudioVideoOffset(float offsetMs);
    float getAudioVideoOffset() const;
    
    // ==========================================================================
    // Power Mode
    // ==========================================================================
    
    void setLowPowerMode(bool enabled);
    bool isLowPowerMode() const;
    
    // ==========================================================================
    // Full Config
    // ==========================================================================
    
    void setConfig(const VideoConfig& config);
    VideoConfig getConfig() const;
    void resetToDefaults();
    
    // ==========================================================================
    // Processing
    // ==========================================================================
    
    /**
     * @brief Process video frame
     * @param input Input frame (RGB)
     * @param output Output buffer
     * @param width Frame width
     * @param height Frame height
     */
    void processFrame(const uint8_t* input, uint8_t* output, 
                      int width, int height);
    
    /**
     * @brief Get processing statistics
     */
    struct Stats {
        float decodeFPS;
        float renderFPS;
        float cpuUsage;
        float gpuUsage;
        bool hardwareDecoding;
        std::string activeDecoder;
        int droppedFrames;
    };
    Stats getStats() const;
    
    // ==========================================================================
    // Callbacks
    // ==========================================================================
    
    void setOnConfigChanged(std::function<void(const VideoConfig&)> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Global video processor instance
 */
VideoProcessor& getVideoProcessor();

} // namespace video
} // namespace zepra

#endif // ZEPRA_VIDEO_PROCESSOR_H
