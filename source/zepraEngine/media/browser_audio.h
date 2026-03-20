// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_BROWSER_AUDIO_H
#define ZEPRA_BROWSER_AUDIO_H

/**
 * @file browser_audio.h
 * @brief Browser audio integration using NXAudio engine
 * 
 * Provides browser-level audio management with:
 * - Spatial audio for immersive experience
 * - Power-efficient processing for laptops/mobile
 * - ARM and x86 SIMD optimizations
 * - Codec detection (Dolby, DTS, AAC, etc.)
 * - Web Audio API compatibility
 * 
 * Platform Support:
 * - NeolyxOS: Full features including hardware decode
 * - Linux: PipeWire/PulseAudio backend
 * - macOS: CoreAudio backend
 * - Windows: WASAPI backend
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace zepra {
namespace audio {

/**
 * @brief Audio quality presets
 */
enum class AudioQuality {
    Eco,        // Low power, basic processing
    Balanced,   // Good quality, moderate power
    High,       // Full quality, spatial audio
    Ultra       // Maximum quality, HRTF enabled
};

/**
 * @brief Audio device info
 */
struct AudioDevice {
    std::string id;
    std::string name;
    bool isDefault;
    bool isInput;
    int channels;
    int sampleRate;
};

/**
 * @brief Spatial audio settings
 */
struct SpatialAudioConfig {
    bool enabled = true;
    bool hrtfEnabled = true;
    float roomSize = 0.5f;
    float reverbAmount = 0.3f;
    float bassBoost = 0.0f;
    bool virtualSurround = false;
};

/**
 * @brief Power/performance mode
 */
enum class PowerMode {
    BatteryOptimized,   // Minimize CPU usage
    Balanced,           // Balance quality/power
    Performance         // Full quality, ignore power
};

/**
 * @brief Audio statistics
 */
struct AudioStats {
    float cpuUsage;         // 0.0 - 1.0
    int activeStreams;
    int buffersInUse;
    float latencyMs;
    bool simdActive;        // Using SIMD optimizations
    std::string activeCodec;
};

/**
 * @brief Browser audio manager
 */
class BrowserAudio {
public:
    BrowserAudio();
    ~BrowserAudio();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // Device management
    std::vector<AudioDevice> getOutputDevices() const;
    std::vector<AudioDevice> getInputDevices() const;
    bool setOutputDevice(const std::string& deviceId);
    bool setInputDevice(const std::string& deviceId);
    std::string getCurrentOutputDevice() const;
    
    // Volume control
    float getMasterVolume() const;
    void setMasterVolume(float volume);  // 0.0 - 1.0
    bool isMuted() const;
    void setMuted(bool muted);
    
    // Quality settings
    AudioQuality getQuality() const;
    void setQuality(AudioQuality quality);
    
    // Spatial audio
    SpatialAudioConfig getSpatialConfig() const;
    void setSpatialConfig(const SpatialAudioConfig& config);
    bool isSpatialAudioSupported() const;
    
    // Power management
    PowerMode getPowerMode() const;
    void setPowerMode(PowerMode mode);
    
    // Tab audio control
    void muteTab(int tabId);
    void unmuteTab(int tabId);
    bool isTabMuted(int tabId) const;
    float getTabVolume(int tabId) const;
    void setTabVolume(int tabId, float volume);
    
    // Statistics
    AudioStats getStats() const;
    
    // Platform info
    bool hasAVX2Support() const;
    bool hasNeonSupport() const;
    bool isArmPlatform() const;
    
    // Callbacks
    void setOnDeviceChange(std::function<void()> callback);
    void setOnVolumeChange(std::function<void(float)> callback);
    
    // Codec detection
    std::string detectCodec(const uint8_t* data, size_t size) const;
    bool isDolbyStream(const uint8_t* data, size_t size) const;
    bool isDTSStream(const uint8_t* data, size_t size) const;
    bool canPassthrough(const std::string& codec) const;
    
    // NeolyxOS-specific (no-op on other platforms)
    bool isNeolyxOS() const;
    void enableHardwareDecode(bool enabled);
    void enableNeolyxSpatial(bool enabled);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Global browser audio instance
 */
BrowserAudio& getBrowserAudio();

} // namespace audio
} // namespace zepra

#endif // ZEPRA_BROWSER_AUDIO_H
