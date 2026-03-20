// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_AUDIO_CODEC_DETECTOR_H
#define ZEPRA_AUDIO_CODEC_DETECTOR_H

/**
 * @file audio_codec_detector.h
 * @brief Audio codec/format detection for browser playback
 * 
 * Detects incoming audio formats (Dolby, DTS, AAC, etc.)
 * and routes to appropriate decoder/processor.
 * 
 * Cross-platform: Linux, macOS, Windows
 * NeolyxOS-specific features in separate section
 */

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace zepra {
namespace audio {

/**
 * @brief Audio codec types
 */
enum class AudioCodec {
    Unknown,
    
    // Lossy compressed
    AAC,            // Advanced Audio Codec
    MP3,            // MPEG Layer 3
    OGG_VORBIS,     // Ogg Vorbis
    OPUS,           // Opus (WebRTC)
    
    // Lossless
    FLAC,           // Free Lossless Audio Codec
    ALAC,           // Apple Lossless
    WAV_PCM,        // Uncompressed WAV
    
    // Surround/Immersive
    DOLBY_DIGITAL,      // AC-3 (5.1)
    DOLBY_DIGITAL_PLUS, // E-AC-3 (7.1)
    DOLBY_ATMOS,        // Dolby Atmos (object-based)
    DOLBY_TRUEHD,       // Lossless
    DTS,                // DTS Core
    DTS_HD,             // DTS-HD Master Audio
    DTS_X,              // DTS:X (object-based)
    
    // Spatial
    AMBISONICS,     // First-order Ambisonics
    BINAURAL,       // Pre-rendered binaural
};

/**
 * @brief Audio container formats
 */
enum class AudioContainer {
    Unknown,
    RAW,        // Raw audio data
    WAV,        // RIFF/WAV
    MP4,        // MPEG-4 Part 14
    MKV,        // Matroska
    WEBM,       // WebM
    OGG,        // Ogg container
    FLAC,       // Native FLAC
    M4A,        // MPEG-4 Audio
    AC3,        // Raw AC-3
    EAC3,       // Raw E-AC-3
    MLP,        // Meridian Lossless Packing (TrueHD)
};

/**
 * @brief Audio stream properties
 */
struct AudioStreamInfo {
    AudioCodec codec = AudioCodec::Unknown;
    AudioContainer container = AudioContainer::Unknown;
    
    // Format details
    uint32_t sampleRate = 0;
    uint8_t channels = 0;
    uint8_t bitsPerSample = 0;
    uint32_t bitrate = 0;
    
    // Channel layout
    bool isStereo = true;
    bool is51 = false;
    bool is71 = false;
    bool isAtmos = false;
    bool isObjectBased = false;
    
    // Duration
    double durationSeconds = 0.0;
    
    // Codec-specific
    std::string codecProfile;
    std::string codecVersion;
};

/**
 * @brief Output device capabilities
 */
struct OutputCapabilities {
    bool supportsStereo = true;
    bool supports51 = false;
    bool supports71 = false;
    bool supportsAtmos = false;
    bool supportsDolbyPassthrough = false;
    bool supportsDTSPassthrough = false;
    bool supportsHiRes = false;  // > 48kHz, > 16-bit
    
    uint32_t maxSampleRate = 48000;
    uint8_t maxBitsPerSample = 16;
    uint8_t maxChannels = 2;
};

/**
 * @brief Processing mode for audio output
 */
enum class ProcessingMode {
    Passthrough,    // Send directly to device (Dolby/DTS)
    Decode,         // Decode to PCM
    Transcode,      // Convert format
    Downmix,        // Reduce channels
    Upmix,          // Virtualize surround
    Spatial,        // Apply HRTF/spatial
};

/**
 * @brief Audio codec detector
 */
class AudioCodecDetector {
public:
    AudioCodecDetector();
    ~AudioCodecDetector();
    
    /**
     * @brief Detect codec from raw audio data
     * @param data Audio data buffer
     * @param size Buffer size in bytes
     * @return Detected audio stream info
     */
    AudioStreamInfo detectFromData(const uint8_t* data, size_t size);
    
    /**
     * @brief Detect codec from file extension and header
     */
    AudioStreamInfo detectFromFile(const std::string& filepath);
    
    /**
     * @brief Detect codec from MIME type
     */
    AudioStreamInfo detectFromMime(const std::string& mimeType);
    
    /**
     * @brief Get recommended processing mode
     */
    ProcessingMode getRecommendedProcessing(
        const AudioStreamInfo& stream,
        const OutputCapabilities& output);
    
    /**
     * @brief Check if codec is supported for playback
     */
    bool isCodecSupported(AudioCodec codec) const;
    
    /**
     * @brief Get human-readable codec name
     */
    static std::string codecName(AudioCodec codec);
    
private:
    // Sync word detection
    bool detectAC3SyncWord(const uint8_t* data, size_t size);
    bool detectEAC3SyncWord(const uint8_t* data, size_t size);
    bool detectDTSSyncWord(const uint8_t* data, size_t size);
    bool detectFLACSyncWord(const uint8_t* data, size_t size);
    bool detectMP3SyncWord(const uint8_t* data, size_t size);
    bool detectAACHeader(const uint8_t* data, size_t size);
    bool detectOggHeader(const uint8_t* data, size_t size);
    bool detectWAVHeader(const uint8_t* data, size_t size);
};

/**
 * @brief Audio output router
 */
class AudioOutputRouter {
public:
    AudioOutputRouter();
    ~AudioOutputRouter();
    
    /**
     * @brief Initialize with output device
     */
    bool initialize();
    
    /**
     * @brief Get output device capabilities
     */
    OutputCapabilities getCapabilities() const;
    
    /**
     * @brief Route audio to appropriate decoder/output
     * @return true if routing successful
     */
    bool routeAudio(const AudioStreamInfo& stream, 
                    const uint8_t* data, size_t size);
    
    /**
     * @brief Set preferred output mode
     */
    void setPreferredMode(ProcessingMode mode);
    
    /**
     * @brief Enable/disable passthrough for Dolby/DTS
     */
    void setPassthroughEnabled(bool enabled);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// =============================================================================
// Platform-Specific APIs
// =============================================================================

#if defined(PLATFORM_NEOLYX)
/**
 * @brief NeolyxOS-specific audio features
 * 
 * These are ONLY available on NeolyxOS, not Linux/Mac/Windows
 */
namespace neolyx {

/**
 * @brief NeolyxOS native audio pipeline
 */
class NativeAudioPipeline {
public:
    // System-level audio routing
    bool setSystemOutput(const std::string& deviceId);
    bool enableSystemMixer(bool enabled);
    
    // Hardware decode offload
    bool enableHardwareDolbyDecode(bool enabled);
    bool enableHardwareDTSDecode(bool enabled);
    
    // NeolyxOS-specific spatial audio
    bool enableNeolyxSpatial(bool enabled);
    
    // Power management integration
    void setAudioPowerProfile(int profile);
};

} // namespace neolyx
#endif

// =============================================================================
// Cross-Platform APIs (Linux, macOS, Windows)
// =============================================================================

namespace crossplatform {

/**
 * @brief Cross-platform audio backend abstraction
 */
enum class AudioBackend {
#if defined(__linux__) && !defined(PLATFORM_NEOLYX)
    PulseAudio,
    PipeWire,
    ALSA,
#endif
#if defined(__APPLE__)
    CoreAudio,
#endif
#if defined(_WIN32)
    WASAPI,
    DirectSound,
#endif
    SDL_Audio,  // Fallback
};

/**
 * @brief Get available audio backend for current platform
 */
AudioBackend getDefaultBackend();

/**
 * @brief Initialize cross-platform audio
 */
bool initializeBackend(AudioBackend backend);

} // namespace crossplatform

} // namespace audio
} // namespace zepra

#endif // ZEPRA_AUDIO_CODEC_DETECTOR_H
