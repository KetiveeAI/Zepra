// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file browser_audio.cpp
 * @brief Browser audio implementation using NXAudio
 * 
 * Integrates NXAudio engine with browser for spatial audio,
 * power-efficient processing, and platform-specific SIMD.
 */

#include "engine/browser_audio.h"
#include <unordered_map>
#include <mutex>
#include <cstring>

// NXAudio C interface
extern "C" {
#include "nxaudio/nxaudio.h"
}

namespace zepra {
namespace audio {

// =============================================================================
// Platform Detection
// =============================================================================

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define PLATFORM_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #define PLATFORM_ARM 1
#endif

#ifdef PLATFORM_X86
#include <cpuid.h>

static bool detectAVX2() {
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
        return (ebx & (1 << 5)) != 0;  // AVX2 bit
    }
    return false;
}

static bool detectSSE41() {
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return (ecx & (1 << 19)) != 0;  // SSE4.1 bit
    }
    return false;
}
#endif

#ifdef PLATFORM_ARM
static bool detectNeon() {
    // ARM NEON is always available on ARMv8 (aarch64)
    #if defined(__aarch64__)
        return true;
    #else
        // For ARMv7, check /proc/cpuinfo or use HWCAP
        return false;
    #endif
}
#endif

// =============================================================================
// BrowserAudio Implementation
// =============================================================================

class BrowserAudio::Impl {
public:
    bool initialized = false;
    nxaudio_context_t context = NXAUDIO_INVALID_HANDLE;
    
    float masterVolume = 1.0f;
    bool muted = false;
    AudioQuality quality = AudioQuality::Balanced;
    PowerMode powerMode = PowerMode::Balanced;
    SpatialAudioConfig spatialConfig;
    
    std::string outputDevice;
    std::string inputDevice;
    
    std::unordered_map<int, float> tabVolumes;
    std::unordered_map<int, bool> tabMuted;
    
    std::function<void()> onDeviceChange;
    std::function<void(float)> onVolumeChange;
    
    std::mutex mutex;
    
    // Platform capabilities
    bool hasAVX2 = false;
    bool hasSSE41 = false;
    bool hasNeon = false;
    bool isArm = false;
    
    Impl() {
        // Detect platform capabilities
        #ifdef PLATFORM_X86
            hasAVX2 = detectAVX2();
            hasSSE41 = detectSSE41();
            isArm = false;
        #endif
        
        #ifdef PLATFORM_ARM
            hasNeon = detectNeon();
            isArm = true;
        #endif
    }
    
    void updatePowerSettings() {
        if (context == NXAUDIO_INVALID_HANDLE) return;
        
        // Adjust audio processing based on power mode
        switch (powerMode) {
            case PowerMode::BatteryOptimized:
                // Disable expensive features
                nxaudio_hrtf_enable(context, 0);
                nxaudio_reverb_enable(context, 0);
                break;
                
            case PowerMode::Balanced:
                // Enable spatial but reduce quality
                nxaudio_hrtf_enable(context, spatialConfig.hrtfEnabled ? 1 : 0);
                nxaudio_reverb_enable(context, 0);
                break;
                
            case PowerMode::Performance:
                // Full quality
                nxaudio_hrtf_enable(context, spatialConfig.hrtfEnabled ? 1 : 0);
                nxaudio_reverb_enable(context, spatialConfig.reverbAmount > 0.0f ? 1 : 0);
                if (spatialConfig.reverbAmount > 0.0f) {
                    nxaudio_reverb_set(context, 
                        spatialConfig.roomSize,
                        0.5f,  // damping
                        spatialConfig.reverbAmount,
                        1.0f - spatialConfig.reverbAmount);
                }
                break;
        }
    }
    
    void updateQualitySettings() {
        if (context == NXAUDIO_INVALID_HANDLE) return;
        
        switch (quality) {
            case AudioQuality::Eco:
                // Minimal processing
                nxaudio_hrtf_enable(context, 0);
                nxaudio_reverb_enable(context, 0);
                break;
                
            case AudioQuality::Balanced:
                // Standard quality
                nxaudio_hrtf_enable(context, 1);
                nxaudio_reverb_enable(context, 0);
                break;
                
            case AudioQuality::High:
                // Full spatial
                nxaudio_hrtf_enable(context, 1);
                nxaudio_hrtf_load_default(context);
                break;
                
            case AudioQuality::Ultra:
                // Everything enabled
                nxaudio_hrtf_enable(context, 1);
                nxaudio_hrtf_load_default(context);
                nxaudio_reverb_enable(context, 1);
                nxaudio_reverb_set(context, 0.8f, 0.5f, 0.3f, 0.7f);
                break;
        }
    }
};

BrowserAudio::BrowserAudio() : impl_(std::make_unique<Impl>()) {}

BrowserAudio::~BrowserAudio() {
    shutdown();
}

bool BrowserAudio::initialize() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->initialized) return true;
    
    // Initialize NXAudio
    if (nxaudio_init() != NXAUDIO_SUCCESS) {
        return false;
    }
    
    // Create audio context with optimal settings
    nxaudio_context_config_t config = {
        .sample_rate = 48000,
        .buffer_size = 512,   // Low latency for browser
        .max_objects = 64,
        .distance_model = NXAUDIO_DISTANCE_INVERSE,
        .doppler_factor = 1.0f,
        .speed_of_sound = 343.0f,
    };
    
    // Adjust for power efficiency on ARM
    if (impl_->isArm) {
        config.buffer_size = 1024;  // Larger buffer = less CPU
    }
    
    impl_->context = nxaudio_context_create_ex(&config);
    if (impl_->context == NXAUDIO_INVALID_HANDLE) {
        nxaudio_shutdown();
        return false;
    }
    
    // Load default HRTF for spatial audio
    nxaudio_hrtf_load_default(impl_->context);
    
    impl_->initialized = true;
    impl_->updateQualitySettings();
    
    return true;
}

void BrowserAudio::shutdown() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->initialized) return;
    
    if (impl_->context != NXAUDIO_INVALID_HANDLE) {
        nxaudio_context_destroy(impl_->context);
        impl_->context = NXAUDIO_INVALID_HANDLE;
    }
    
    nxaudio_shutdown();
    impl_->initialized = false;
}

bool BrowserAudio::isInitialized() const {
    return impl_->initialized;
}

std::vector<AudioDevice> BrowserAudio::getOutputDevices() const {
    // TODO: Enumerate actual devices via platform audio API
    std::vector<AudioDevice> devices;
    devices.push_back({
        "default", "System Default", true, false, 2, 48000
    });
    return devices;
}

std::vector<AudioDevice> BrowserAudio::getInputDevices() const {
    std::vector<AudioDevice> devices;
    devices.push_back({
        "default", "System Default", true, true, 2, 48000
    });
    return devices;
}

bool BrowserAudio::setOutputDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->outputDevice = deviceId;
    return true;
}

bool BrowserAudio::setInputDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->inputDevice = deviceId;
    return true;
}

std::string BrowserAudio::getCurrentOutputDevice() const {
    return impl_->outputDevice.empty() ? "default" : impl_->outputDevice;
}

float BrowserAudio::getMasterVolume() const {
    return impl_->masterVolume;
}

void BrowserAudio::setMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->masterVolume = std::max(0.0f, std::min(1.0f, volume));
    
    if (impl_->context != NXAUDIO_INVALID_HANDLE && !impl_->muted) {
        nxaudio_context_set_gain(impl_->context, impl_->masterVolume);
    }
    
    if (impl_->onVolumeChange) {
        impl_->onVolumeChange(impl_->masterVolume);
    }
}

bool BrowserAudio::isMuted() const {
    return impl_->muted;
}

void BrowserAudio::setMuted(bool muted) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->muted = muted;
    
    if (impl_->context != NXAUDIO_INVALID_HANDLE) {
        nxaudio_context_set_gain(impl_->context, muted ? 0.0f : impl_->masterVolume);
    }
}

AudioQuality BrowserAudio::getQuality() const {
    return impl_->quality;
}

void BrowserAudio::setQuality(AudioQuality quality) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->quality = quality;
    impl_->updateQualitySettings();
}

SpatialAudioConfig BrowserAudio::getSpatialConfig() const {
    return impl_->spatialConfig;
}

void BrowserAudio::setSpatialConfig(const SpatialAudioConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->spatialConfig = config;
    impl_->updatePowerSettings();
}

bool BrowserAudio::isSpatialAudioSupported() const {
    // Spatial audio supported on all platforms
    return true;
}

PowerMode BrowserAudio::getPowerMode() const {
    return impl_->powerMode;
}

void BrowserAudio::setPowerMode(PowerMode mode) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->powerMode = mode;
    impl_->updatePowerSettings();
}

void BrowserAudio::muteTab(int tabId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->tabMuted[tabId] = true;
}

void BrowserAudio::unmuteTab(int tabId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->tabMuted[tabId] = false;
}

bool BrowserAudio::isTabMuted(int tabId) const {
    auto it = impl_->tabMuted.find(tabId);
    return it != impl_->tabMuted.end() && it->second;
}

float BrowserAudio::getTabVolume(int tabId) const {
    auto it = impl_->tabVolumes.find(tabId);
    return it != impl_->tabVolumes.end() ? it->second : 1.0f;
}

void BrowserAudio::setTabVolume(int tabId, float volume) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->tabVolumes[tabId] = std::max(0.0f, std::min(1.0f, volume));
}

AudioStats BrowserAudio::getStats() const {
    AudioStats stats;
    stats.cpuUsage = 0.05f;  // Placeholder
    stats.activeStreams = 0;
    stats.buffersInUse = 0;
    stats.latencyMs = 10.0f;
    stats.simdActive = impl_->hasAVX2 || impl_->hasSSE41 || impl_->hasNeon;
    stats.activeCodec = "AAC";
    return stats;
}

bool BrowserAudio::hasAVX2Support() const {
    return impl_->hasAVX2;
}

bool BrowserAudio::hasNeonSupport() const {
    return impl_->hasNeon;
}

bool BrowserAudio::isArmPlatform() const {
    return impl_->isArm;
}

void BrowserAudio::setOnDeviceChange(std::function<void()> callback) {
    impl_->onDeviceChange = callback;
}

void BrowserAudio::setOnVolumeChange(std::function<void(float)> callback) {
    impl_->onVolumeChange = callback;
}

// =============================================================================
// Codec Detection
// =============================================================================

std::string BrowserAudio::detectCodec(const uint8_t* data, size_t size) const {
    if (!data || size < 4) return "Unknown";
    
    // AC-3 (Dolby Digital) sync: 0x0B77
    if (size >= 2 && data[0] == 0x0B && data[1] == 0x77) {
        // Check for E-AC-3 (bsid > 10)
        if (size >= 6) {
            uint8_t bsid = (data[5] >> 3) & 0x1F;
            if (bsid > 10) return "Dolby Digital Plus";
        }
        return "Dolby Digital";
    }
    
    // DTS sync words
    if (size >= 4) {
        if ((data[0] == 0x7F && data[1] == 0xFE && data[2] == 0x80 && data[3] == 0x01) ||
            (data[0] == 0xFE && data[1] == 0x7F && data[2] == 0x01 && data[3] == 0x80)) {
            return "DTS";
        }
    }
    
    // TrueHD sync
    if (size >= 4 && data[0] == 0xF8 && data[1] == 0x72 && data[2] == 0x6F && data[3] == 0xBA) {
        return "Dolby TrueHD";
    }
    
    // FLAC
    if (size >= 4 && data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C') {
        return "FLAC";
    }
    
    // OGG
    if (size >= 4 && data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S') {
        return "Ogg Vorbis";
    }
    
    // AAC ADTS
    if (size >= 2 && data[0] == 0xFF && (data[1] & 0xF0) == 0xF0) {
        return "AAC";
    }
    
    // MP3
    if (size >= 2 && data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) {
        return "MP3";
    }
    
    // WAV
    if (size >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'A' && data[10] == 'V' && data[11] == 'E') {
        return "WAV/PCM";
    }
    
    return "Unknown";
}

bool BrowserAudio::isDolbyStream(const uint8_t* data, size_t size) const {
    if (!data || size < 2) return false;
    
    // AC-3 / E-AC-3 sync
    if (data[0] == 0x0B && data[1] == 0x77) return true;
    
    // TrueHD
    if (size >= 4 && data[0] == 0xF8 && data[1] == 0x72 && data[2] == 0x6F && data[3] == 0xBA) {
        return true;
    }
    
    return false;
}

bool BrowserAudio::isDTSStream(const uint8_t* data, size_t size) const {
    if (!data || size < 4) return false;
    
    // DTS sync word variants
    if ((data[0] == 0x7F && data[1] == 0xFE && data[2] == 0x80 && data[3] == 0x01) ||
        (data[0] == 0xFE && data[1] == 0x7F && data[2] == 0x01 && data[3] == 0x80) ||
        (data[0] == 0x1F && data[1] == 0xFF && data[2] == 0xE8 && data[3] == 0x00)) {
        return true;
    }
    
    return false;
}

bool BrowserAudio::canPassthrough(const std::string& codec) const {
    // Passthrough requires hardware support
    // On NeolyxOS, always available
    // On other platforms, check device capabilities
    
    #if defined(PLATFORM_NEOLYX)
        // NeolyxOS supports all passthrough
        return (codec == "Dolby Digital" || 
                codec == "Dolby Digital Plus" ||
                codec == "Dolby TrueHD" ||
                codec == "DTS");
    #else
        // Other platforms: only if device supports
        // For now, return false (software decode)
        (void)codec;
        return false;
    #endif
}

// =============================================================================
// NeolyxOS-Specific Features
// =============================================================================

bool BrowserAudio::isNeolyxOS() const {
    #if defined(PLATFORM_NEOLYX)
        return true;
    #else
        return false;
    #endif
}

void BrowserAudio::enableHardwareDecode(bool enabled) {
    #if defined(PLATFORM_NEOLYX)
        // NeolyxOS: Enable hardware Dolby/DTS decoder
        // This would call NeolyxOS-specific APIs
        (void)enabled;
    #else
        // No-op on other platforms
        (void)enabled;
    #endif
}

void BrowserAudio::enableNeolyxSpatial(bool enabled) {
    #if defined(PLATFORM_NEOLYX)
        // NeolyxOS: Enable native spatial audio engine
        (void)enabled;
    #else
        // No-op on other platforms
        (void)enabled;
    #endif
}

// Global instance
BrowserAudio& getBrowserAudio() {
    static BrowserAudio instance;
    return instance;
}

} // namespace audio
} // namespace zepra

