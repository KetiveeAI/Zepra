// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file audio_codec_detector.cpp
 * @brief Audio codec detection implementation
 * 
 * Detects Dolby, DTS, AAC, and other audio formats from sync words.
 * Routes audio to appropriate decoder based on format and output device.
 */

#include "engine/audio_codec_detector.h"
#include <cstring>
#include <algorithm>
#include <fstream>

namespace zepra {
namespace audio {

// =============================================================================
// Sync Word Constants
// =============================================================================

// AC-3 (Dolby Digital) sync word: 0x0B77
static const uint8_t AC3_SYNC[] = {0x0B, 0x77};

// E-AC-3 (Dolby Digital Plus) - same sync but different BSI
static const uint8_t EAC3_SYNC[] = {0x0B, 0x77};

// DTS sync words (big/little endian variants)
static const uint8_t DTS_SYNC_BE[] = {0x7F, 0xFE, 0x80, 0x01};
static const uint8_t DTS_SYNC_LE[] = {0xFE, 0x7F, 0x01, 0x80};
static const uint8_t DTS_SYNC_14BE[] = {0x1F, 0xFF, 0xE8, 0x00};
static const uint8_t DTS_SYNC_14LE[] = {0xFF, 0x1F, 0x00, 0xE8};

// FLAC sync word
static const uint8_t FLAC_SYNC[] = {0x66, 0x4C, 0x61, 0x43}; // "fLaC"

// OGG magic
static const uint8_t OGG_MAGIC[] = {0x4F, 0x67, 0x67, 0x53}; // "OggS"

// WAV/RIFF magic
static const uint8_t RIFF_MAGIC[] = {0x52, 0x49, 0x46, 0x46}; // "RIFF"
static const uint8_t WAVE_MAGIC[] = {0x57, 0x41, 0x56, 0x45}; // "WAVE"

// MP3 frame sync: 0xFF 0xFB/FA/FB
static const uint8_t MP3_SYNC = 0xFF;
static const uint8_t MP3_FRAME_MASK = 0xE0; // First 3 bits = 1

// AAC ADTS sync: 0xFFF (12 bits)
static const uint16_t AAC_ADTS_SYNC = 0xFFF0;

// Dolby TrueHD/MLP sync
static const uint8_t TRUEHD_SYNC[] = {0xF8, 0x72, 0x6F, 0xBA};

// =============================================================================
// AudioCodecDetector Implementation
// =============================================================================

AudioCodecDetector::AudioCodecDetector() = default;
AudioCodecDetector::~AudioCodecDetector() = default;

bool AudioCodecDetector::detectAC3SyncWord(const uint8_t* data, size_t size) {
    if (size < 2) return false;
    return data[0] == AC3_SYNC[0] && data[1] == AC3_SYNC[1];
}

bool AudioCodecDetector::detectEAC3SyncWord(const uint8_t* data, size_t size) {
    // E-AC-3 has same sync as AC-3, differentiated by BSI
    if (size < 6) return false;
    if (data[0] != EAC3_SYNC[0] || data[1] != EAC3_SYNC[1]) return false;
    
    // Check bsid (bit stream identification) for E-AC-3 (16)
    // BSI is at bits 40-44 of the frame
    uint8_t bsid = (data[5] >> 3) & 0x1F;
    return bsid > 10;  // bsid > 10 indicates E-AC-3
}

bool AudioCodecDetector::detectDTSSyncWord(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    
    // Check all DTS sync word variants
    if (memcmp(data, DTS_SYNC_BE, 4) == 0) return true;
    if (memcmp(data, DTS_SYNC_LE, 4) == 0) return true;
    if (memcmp(data, DTS_SYNC_14BE, 4) == 0) return true;
    if (memcmp(data, DTS_SYNC_14LE, 4) == 0) return true;
    
    return false;
}

bool AudioCodecDetector::detectFLACSyncWord(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    return memcmp(data, FLAC_SYNC, 4) == 0;
}

bool AudioCodecDetector::detectMP3SyncWord(const uint8_t* data, size_t size) {
    if (size < 2) return false;
    
    // MP3 frame sync: first 11 bits are 1
    if (data[0] != MP3_SYNC) return false;
    if ((data[1] & MP3_FRAME_MASK) != MP3_FRAME_MASK) return false;
    
    // Additional checks for valid MPEG audio
    uint8_t version = (data[1] >> 3) & 0x03;
    uint8_t layer = (data[1] >> 1) & 0x03;
    
    return version != 0x01 && layer != 0x00;  // Reserved values
}

bool AudioCodecDetector::detectAACHeader(const uint8_t* data, size_t size) {
    if (size < 2) return false;
    
    // ADTS header: 0xFFF (12 bits)
    uint16_t sync = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    return (sync & 0xFFF0) == 0xFFF0;
}

bool AudioCodecDetector::detectOggHeader(const uint8_t* data, size_t size) {
    if (size < 4) return false;
    return memcmp(data, OGG_MAGIC, 4) == 0;
}

bool AudioCodecDetector::detectWAVHeader(const uint8_t* data, size_t size) {
    if (size < 12) return false;
    return memcmp(data, RIFF_MAGIC, 4) == 0 && 
           memcmp(data + 8, WAVE_MAGIC, 4) == 0;
}

AudioStreamInfo AudioCodecDetector::detectFromData(const uint8_t* data, size_t size) {
    AudioStreamInfo info;
    
    if (!data || size < 4) {
        return info;
    }
    
    // Detect container first
    if (detectWAVHeader(data, size)) {
        info.container = AudioContainer::WAV;
        info.codec = AudioCodec::WAV_PCM;
        
        // Parse WAV header for format info
        if (size >= 44) {
            info.channels = data[22] | (data[23] << 8);
            info.sampleRate = data[24] | (data[25] << 8) | 
                             (data[26] << 16) | (data[27] << 24);
            info.bitsPerSample = data[34] | (data[35] << 8);
            info.isStereo = (info.channels == 2);
        }
        return info;
    }
    
    if (detectOggHeader(data, size)) {
        info.container = AudioContainer::OGG;
        // Could be Vorbis or Opus - check further
        if (size >= 36 && memcmp(data + 29, "vorbis", 6) == 0) {
            info.codec = AudioCodec::OGG_VORBIS;
        } else if (size >= 36 && memcmp(data + 28, "OpusHead", 8) == 0) {
            info.codec = AudioCodec::OPUS;
        } else {
            info.codec = AudioCodec::OGG_VORBIS;  // Default
        }
        return info;
    }
    
    if (detectFLACSyncWord(data, size)) {
        info.container = AudioContainer::FLAC;
        info.codec = AudioCodec::FLAC;
        return info;
    }
    
    // Surround formats (usually inside containers but check raw)
    if (detectEAC3SyncWord(data, size)) {
        info.container = AudioContainer::EAC3;
        info.codec = AudioCodec::DOLBY_DIGITAL_PLUS;
        info.is71 = true;  // E-AC-3 typically 7.1
        return info;
    }
    
    if (detectAC3SyncWord(data, size)) {
        info.container = AudioContainer::AC3;
        info.codec = AudioCodec::DOLBY_DIGITAL;
        info.is51 = true;  // AC-3 typically 5.1
        return info;
    }
    
    if (detectDTSSyncWord(data, size)) {
        info.codec = AudioCodec::DTS;
        info.is51 = true;
        return info;
    }
    
    // Check for TrueHD/MLP
    if (size >= 4 && memcmp(data, TRUEHD_SYNC, 4) == 0) {
        info.codec = AudioCodec::DOLBY_TRUEHD;
        info.is71 = true;
        return info;
    }
    
    // Compressed formats
    if (detectAACHeader(data, size)) {
        info.codec = AudioCodec::AAC;
        return info;
    }
    
    if (detectMP3SyncWord(data, size)) {
        info.codec = AudioCodec::MP3;
        return info;
    }
    
    return info;
}

AudioStreamInfo AudioCodecDetector::detectFromMime(const std::string& mimeType) {
    AudioStreamInfo info;
    
    // Map MIME types to codecs
    static const struct {
        const char* mime;
        AudioCodec codec;
        AudioContainer container;
    } mimeMap[] = {
        {"audio/mpeg", AudioCodec::MP3, AudioContainer::RAW},
        {"audio/mp3", AudioCodec::MP3, AudioContainer::RAW},
        {"audio/aac", AudioCodec::AAC, AudioContainer::RAW},
        {"audio/mp4", AudioCodec::AAC, AudioContainer::MP4},
        {"audio/x-m4a", AudioCodec::AAC, AudioContainer::M4A},
        {"audio/ogg", AudioCodec::OGG_VORBIS, AudioContainer::OGG},
        {"audio/opus", AudioCodec::OPUS, AudioContainer::OGG},
        {"audio/webm", AudioCodec::OPUS, AudioContainer::WEBM},
        {"audio/flac", AudioCodec::FLAC, AudioContainer::FLAC},
        {"audio/x-flac", AudioCodec::FLAC, AudioContainer::FLAC},
        {"audio/wav", AudioCodec::WAV_PCM, AudioContainer::WAV},
        {"audio/x-wav", AudioCodec::WAV_PCM, AudioContainer::WAV},
        {"audio/ac3", AudioCodec::DOLBY_DIGITAL, AudioContainer::AC3},
        {"audio/eac3", AudioCodec::DOLBY_DIGITAL_PLUS, AudioContainer::EAC3},
        {"audio/vnd.dts", AudioCodec::DTS, AudioContainer::RAW},
        {"audio/vnd.dolby.mlp", AudioCodec::DOLBY_TRUEHD, AudioContainer::MLP},
    };
    
    for (const auto& m : mimeMap) {
        if (mimeType == m.mime) {
            info.codec = m.codec;
            info.container = m.container;
            break;
        }
    }
    
    return info;
}

AudioStreamInfo AudioCodecDetector::detectFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return AudioStreamInfo();
    }
    
    // Read header
    uint8_t header[128];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    size_t bytesRead = file.gcount();
    
    return detectFromData(header, bytesRead);
}

ProcessingMode AudioCodecDetector::getRecommendedProcessing(
    const AudioStreamInfo& stream,
    const OutputCapabilities& output) {
    
    // Passthrough for Dolby/DTS if supported
    if ((stream.codec == AudioCodec::DOLBY_DIGITAL || 
         stream.codec == AudioCodec::DOLBY_DIGITAL_PLUS) &&
        output.supportsDolbyPassthrough) {
        return ProcessingMode::Passthrough;
    }
    
    if ((stream.codec == AudioCodec::DTS || 
         stream.codec == AudioCodec::DTS_HD) &&
        output.supportsDTSPassthrough) {
        return ProcessingMode::Passthrough;
    }
    
    // Downmix if surround but output is stereo
    if ((stream.is51 || stream.is71) && !output.supports51) {
        return ProcessingMode::Downmix;
    }
    
    // Spatial processing for stereo headphones
    if (!output.supports51 && (stream.is51 || stream.is71 || stream.isAtmos)) {
        return ProcessingMode::Spatial;
    }
    
    // Default: decode to PCM
    return ProcessingMode::Decode;
}

bool AudioCodecDetector::isCodecSupported(AudioCodec codec) const {
    // All common codecs supported (software decode)
    switch (codec) {
        case AudioCodec::AAC:
        case AudioCodec::MP3:
        case AudioCodec::OGG_VORBIS:
        case AudioCodec::OPUS:
        case AudioCodec::FLAC:
        case AudioCodec::WAV_PCM:
        case AudioCodec::DOLBY_DIGITAL:
        case AudioCodec::DOLBY_DIGITAL_PLUS:
        case AudioCodec::DTS:
            return true;
            
        case AudioCodec::DOLBY_ATMOS:
        case AudioCodec::DOLBY_TRUEHD:
        case AudioCodec::DTS_HD:
        case AudioCodec::DTS_X:
            // Require hardware or special decoder
            return false;
            
        default:
            return false;
    }
}

std::string AudioCodecDetector::codecName(AudioCodec codec) {
    switch (codec) {
        case AudioCodec::AAC: return "AAC";
        case AudioCodec::MP3: return "MP3";
        case AudioCodec::OGG_VORBIS: return "Ogg Vorbis";
        case AudioCodec::OPUS: return "Opus";
        case AudioCodec::FLAC: return "FLAC";
        case AudioCodec::ALAC: return "ALAC";
        case AudioCodec::WAV_PCM: return "PCM/WAV";
        case AudioCodec::DOLBY_DIGITAL: return "Dolby Digital (AC-3)";
        case AudioCodec::DOLBY_DIGITAL_PLUS: return "Dolby Digital Plus (E-AC-3)";
        case AudioCodec::DOLBY_ATMOS: return "Dolby Atmos";
        case AudioCodec::DOLBY_TRUEHD: return "Dolby TrueHD";
        case AudioCodec::DTS: return "DTS";
        case AudioCodec::DTS_HD: return "DTS-HD Master Audio";
        case AudioCodec::DTS_X: return "DTS:X";
        case AudioCodec::AMBISONICS: return "Ambisonics";
        case AudioCodec::BINAURAL: return "Binaural";
        default: return "Unknown";
    }
}

// =============================================================================
// AudioOutputRouter Implementation
// =============================================================================

class AudioOutputRouter::Impl {
public:
    OutputCapabilities capabilities;
    ProcessingMode preferredMode = ProcessingMode::Decode;
    bool passthroughEnabled = true;
    bool initialized = false;
    
    void detectCapabilities() {
        // Platform-specific capability detection
        capabilities.supportsStereo = true;
        capabilities.maxSampleRate = 48000;
        capabilities.maxBitsPerSample = 16;
        capabilities.maxChannels = 2;
        
        // TODO: Query actual hardware capabilities
        // On desktop, assume basic stereo for now
        #if defined(PLATFORM_NEOLYX)
            // NeolyxOS has full surround support
            capabilities.supports51 = true;
            capabilities.supports71 = true;
            capabilities.supportsAtmos = true;
            capabilities.supportsDolbyPassthrough = true;
            capabilities.supportsDTSPassthrough = true;
            capabilities.supportsHiRes = true;
            capabilities.maxSampleRate = 192000;
            capabilities.maxBitsPerSample = 24;
            capabilities.maxChannels = 8;
        #endif
    }
};

AudioOutputRouter::AudioOutputRouter() : impl_(std::make_unique<Impl>()) {}
AudioOutputRouter::~AudioOutputRouter() = default;

bool AudioOutputRouter::initialize() {
    impl_->detectCapabilities();
    impl_->initialized = true;
    return true;
}

OutputCapabilities AudioOutputRouter::getCapabilities() const {
    return impl_->capabilities;
}

bool AudioOutputRouter::routeAudio(const AudioStreamInfo& stream,
                                    const uint8_t* data, size_t size) {
    if (!impl_->initialized) return false;
    
    AudioCodecDetector detector;
    ProcessingMode mode = detector.getRecommendedProcessing(
        stream, impl_->capabilities);
    
    if (impl_->preferredMode != ProcessingMode::Decode) {
        mode = impl_->preferredMode;
    }
    
    // Route based on mode
    switch (mode) {
        case ProcessingMode::Passthrough:
            // Send directly to audio driver
            // TODO: Implement passthrough
            break;
            
        case ProcessingMode::Decode:
            // Decode to PCM and play
            // TODO: Implement decoding
            break;
            
        case ProcessingMode::Downmix:
            // Decode and downmix to stereo
            // TODO: Implement downmix
            break;
            
        case ProcessingMode::Spatial:
            // Apply HRTF for virtual surround
            // TODO: Implement spatial
            break;
            
        default:
            break;
    }
    
    return true;
}

void AudioOutputRouter::setPreferredMode(ProcessingMode mode) {
    impl_->preferredMode = mode;
}

void AudioOutputRouter::setPassthroughEnabled(bool enabled) {
    impl_->passthroughEnabled = enabled;
}

// =============================================================================
// Cross-Platform Backend
// =============================================================================

namespace crossplatform {

AudioBackend getDefaultBackend() {
#if defined(__linux__) && !defined(PLATFORM_NEOLYX)
    // Prefer PipeWire, fall back to PulseAudio
    return AudioBackend::PipeWire;
#elif defined(__APPLE__)
    return AudioBackend::CoreAudio;
#elif defined(_WIN32)
    return AudioBackend::WASAPI;
#else
    return AudioBackend::SDL_Audio;
#endif
}

bool initializeBackend(AudioBackend backend) {
    // TODO: Initialize selected backend
    (void)backend;
    return true;
}

} // namespace crossplatform

} // namespace audio
} // namespace zepra
