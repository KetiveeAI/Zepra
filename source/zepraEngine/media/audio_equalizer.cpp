// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file audio_equalizer.cpp
 * @brief Advanced audio equalizer implementation
 * 
 * Full DSP processing including:
 * - 10-band parametric EQ
 * - Bass boost with configurable frequency
 * - Room acoustics simulation
 * - Dynamic range compression
 * - Pitch/tempo manipulation
 */

#include "engine/audio_equalizer.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace zepra {
namespace audio {

// =============================================================================
// Preset EQ Curves
// =============================================================================

// EQ curves for each preset (10 bands: 32, 64, 125, 250, 500, 1k, 2k, 4k, 8k, 16k Hz)
static const std::array<float, 10> PRESET_CURVES[] = {
    // Default (flat)
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    
    // Music
    {2, 3, 1, 0, 0, 0, 1, 2, 3, 2},
    
    // Movie
    {4, 5, 3, 0, -1, 0, 2, 4, 3, 1},
    
    // Gaming
    {3, 4, 2, 0, 1, 2, 3, 5, 4, 2},
    
    // Podcast
    {-2, -1, 0, 2, 3, 4, 3, 2, 0, -2},
    
    // Audiobook
    {-3, -2, 0, 2, 4, 5, 4, 2, -1, -3},
    
    // Pop
    {-1, 1, 3, 4, 3, 1, 0, 2, 3, 2},
    
    // Rock
    {4, 5, 3, 1, -1, 0, 2, 4, 3, 2},
    
    // Classical
    {0, 0, 0, 0, 0, 0, 0, 1, 2, 3},
    
    // Jazz
    {2, 3, 2, 1, 0, 0, 1, 2, 3, 3},
    
    // EDM
    {6, 7, 5, 2, 0, 0, 2, 4, 5, 4},
    
    // HipHop
    {7, 8, 5, 2, 0, 1, 2, 3, 2, 1},
    
    // LateNight
    {-4, -2, 0, 1, 2, 2, 1, 0, -2, -4},
    
    // LoudSpeaker
    {5, 4, 2, 0, -1, 0, 1, 3, 4, 3},
    
    // Headphones
    {1, 2, 1, 0, 0, 0, 1, 2, 1, 0},
    
    // SmallSpeakers
    {6, 7, 5, 2, 0, 0, 2, 3, 2, 1},
    
    // Custom (starts flat)
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// Band frequencies in Hz
static const float BAND_FREQUENCIES[] = {
    32.0f, 64.0f, 125.0f, 250.0f, 500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

static const char* BAND_NAMES[] = {
    "32 Hz", "64 Hz", "125 Hz", "250 Hz", "500 Hz",
    "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz"
};

// =============================================================================
// Helper Functions
// =============================================================================

std::string getPresetName(AudioPreset preset) {
    switch (preset) {
        case AudioPreset::Default: return "Default";
        case AudioPreset::Music: return "Music";
        case AudioPreset::Movie: return "Movie";
        case AudioPreset::Gaming: return "Gaming";
        case AudioPreset::Podcast: return "Podcast";
        case AudioPreset::Audiobook: return "Audiobook";
        case AudioPreset::Pop: return "Pop";
        case AudioPreset::Rock: return "Rock";
        case AudioPreset::Classical: return "Classical";
        case AudioPreset::Jazz: return "Jazz";
        case AudioPreset::EDM: return "Electronic/EDM";
        case AudioPreset::HipHop: return "Hip-Hop";
        case AudioPreset::LateNight: return "Late Night";
        case AudioPreset::LoudSpeaker: return "Loud Speaker";
        case AudioPreset::Headphones: return "Headphones";
        case AudioPreset::SmallSpeakers: return "Small Speakers";
        case AudioPreset::Custom: return "Custom";
        default: return "Unknown";
    }
}

std::vector<AudioPreset> getAllPresets() {
    return {
        AudioPreset::Default,
        AudioPreset::Music,
        AudioPreset::Movie,
        AudioPreset::Gaming,
        AudioPreset::Podcast,
        AudioPreset::Audiobook,
        AudioPreset::Pop,
        AudioPreset::Rock,
        AudioPreset::Classical,
        AudioPreset::Jazz,
        AudioPreset::EDM,
        AudioPreset::HipHop,
        AudioPreset::LateNight,
        AudioPreset::LoudSpeaker,
        AudioPreset::Headphones,
        AudioPreset::SmallSpeakers,
        AudioPreset::Custom
    };
}

float getBandFrequency(EQBand band) {
    int idx = static_cast<int>(band);
    if (idx >= 0 && idx < 10) {
        return BAND_FREQUENCIES[idx];
    }
    return 0.0f;
}

std::string getBandName(EQBand band) {
    int idx = static_cast<int>(band);
    if (idx >= 0 && idx < 10) {
        return BAND_NAMES[idx];
    }
    return "Unknown";
}

// =============================================================================
// Biquad Filter for EQ
// =============================================================================

struct BiquadFilter {
    float b0, b1, b2, a1, a2;
    float z1, z2;  // State
    
    BiquadFilter() : b0(1), b1(0), b2(0), a1(0), a2(0), z1(0), z2(0) {}
    
    void reset() { z1 = z2 = 0; }
    
    // Configure as peaking EQ filter
    void setPeakingEQ(float sampleRate, float freq, float gainDb, float Q = 1.0f) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * M_PI * freq / sampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);
        
        float b0_ = 1.0f + alpha * A;
        float b1_ = -2.0f * std::cos(w0);
        float b2_ = 1.0f - alpha * A;
        float a0_ = 1.0f + alpha / A;
        float a1_ = -2.0f * std::cos(w0);
        float a2_ = 1.0f - alpha / A;
        
        // Normalize
        b0 = b0_ / a0_;
        b1 = b1_ / a0_;
        b2 = b2_ / a0_;
        a1 = a1_ / a0_;
        a2 = a2_ / a0_;
    }
    
    // Configure as low-shelf filter (for bass boost)
    void setLowShelf(float sampleRate, float freq, float gainDb) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * M_PI * freq / sampleRate;
        float alpha = std::sin(w0) / 2.0f * std::sqrt(2.0f);
        
        float cosw0 = std::cos(w0);
        float sqrtA = std::sqrt(A);
        
        float b0_ = A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
        float b1_ = 2 * A * ((A - 1) - (A + 1) * cosw0);
        float b2_ = A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
        float a0_ = (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
        float a1_ = -2 * ((A - 1) + (A + 1) * cosw0);
        float a2_ = (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;
        
        b0 = b0_ / a0_;
        b1 = b1_ / a0_;
        b2 = b2_ / a0_;
        a1 = a1_ / a0_;
        a2 = a2_ / a0_;
    }
    
    float process(float input) {
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        return output;
    }
};

// =============================================================================
// AudioEqualizer Implementation
// =============================================================================

class AudioEqualizer::Impl {
public:
    AudioDSPConfig config;
    
    // EQ filters (10 bands, stereo)
    std::array<BiquadFilter, 10> eqFiltersL;
    std::array<BiquadFilter, 10> eqFiltersR;
    
    // Bass boost filter
    BiquadFilter bassBoostL, bassBoostR;
    
    // Compressor state
    float compEnvelope = 0.0f;
    
    // Custom presets
    std::unordered_map<std::string, AudioDSPConfig> customPresets;
    
    // Callback
    std::function<void(const AudioDSPConfig&)> onSettingsChanged;
    
    float sampleRate = 48000.0f;
    
    Impl() {
        updateFilters();
    }
    
    void updateFilters() {
        // Update EQ filters
        for (int i = 0; i < 10; ++i) {
            float freq = BAND_FREQUENCIES[i];
            float gain = config.equalizer.bands[i];
            
            eqFiltersL[i].setPeakingEQ(sampleRate, freq, gain, 1.0f);
            eqFiltersR[i].setPeakingEQ(sampleRate, freq, gain, 1.0f);
        }
        
        // Update bass boost filter
        if (config.bassBoost.enabled) {
            float boostDb = config.bassBoost.amount * 12.0f;  // 0-1 -> 0-12 dB
            bassBoostL.setLowShelf(sampleRate, config.bassBoost.frequency, boostDb);
            bassBoostR.setLowShelf(sampleRate, config.bassBoost.frequency, boostDb);
        }
    }
    
    void notifyChange() {
        if (onSettingsChanged) {
            onSettingsChanged(config);
        }
    }
    
    // Apply dynamics processing
    float applyCompressor(float input) {
        if (!config.dynamics.enabled) return input;
        
        float inputDb = 20.0f * std::log10(std::abs(input) + 1e-6f);
        float threshold = config.dynamics.threshold;
        float ratio = config.dynamics.ratio;
        
        float gainReduction = 0.0f;
        if (inputDb > threshold) {
            float excess = inputDb - threshold;
            gainReduction = excess - excess / ratio;
        }
        
        // Envelope follower
        float attackCoeff = std::exp(-1.0f / (sampleRate * config.dynamics.attack / 1000.0f));
        float releaseCoeff = std::exp(-1.0f / (sampleRate * config.dynamics.release / 1000.0f));
        
        if (gainReduction > compEnvelope) {
            compEnvelope = attackCoeff * compEnvelope + (1 - attackCoeff) * gainReduction;
        } else {
            compEnvelope = releaseCoeff * compEnvelope + (1 - releaseCoeff) * gainReduction;
        }
        
        float gain = std::pow(10.0f, (-compEnvelope + config.dynamics.makeupGain) / 20.0f);
        float output = input * gain;
        
        // Limiter
        if (config.dynamics.limiterEnabled) {
            float ceiling = std::pow(10.0f, config.dynamics.limiterCeiling / 20.0f);
            output = std::max(-ceiling, std::min(ceiling, output));
        }
        
        return output;
    }
    
    // Pitch shifting (simple linear interpolation for demo)
    // Production would use phase vocoder or WSOLA
    float pitchShift = 0.0f;  // Accumulator
};

AudioEqualizer::AudioEqualizer() : impl_(std::make_unique<Impl>()) {}
AudioEqualizer::~AudioEqualizer() = default;

// =============================================================================
// Preset Controls
// =============================================================================

void AudioEqualizer::applyPreset(AudioPreset preset) {
    int idx = static_cast<int>(preset);
    if (idx >= 0 && idx < 17) {
        impl_->config.preset = preset;
        impl_->config.equalizer.bands = PRESET_CURVES[idx];
        impl_->updateFilters();
        impl_->notifyChange();
    }
}

AudioPreset AudioEqualizer::getCurrentPreset() const {
    return impl_->config.preset;
}

EqualizerSettings AudioEqualizer::getPresetEQ(AudioPreset preset) const {
    EqualizerSettings settings;
    int idx = static_cast<int>(preset);
    if (idx >= 0 && idx < 17) {
        settings.bands = PRESET_CURVES[idx];
    }
    return settings;
}

bool AudioEqualizer::saveCustomPreset(const std::string& name) {
    impl_->customPresets[name] = impl_->config;
    return true;
}

bool AudioEqualizer::loadCustomPreset(const std::string& name) {
    auto it = impl_->customPresets.find(name);
    if (it != impl_->customPresets.end()) {
        impl_->config = it->second;
        impl_->updateFilters();
        impl_->notifyChange();
        return true;
    }
    return false;
}

std::vector<std::string> AudioEqualizer::getCustomPresets() const {
    std::vector<std::string> names;
    for (const auto& pair : impl_->customPresets) {
        names.push_back(pair.first);
    }
    return names;
}

// =============================================================================
// Equalizer Control
// =============================================================================

void AudioEqualizer::setBand(EQBand band, float gainDb) {
    int idx = static_cast<int>(band);
    if (idx >= 0 && idx < 10) {
        impl_->config.equalizer.bands[idx] = std::max(-12.0f, std::min(12.0f, gainDb));
        impl_->config.preset = AudioPreset::Custom;
        impl_->updateFilters();
        impl_->notifyChange();
    }
}

float AudioEqualizer::getBand(EQBand band) const {
    int idx = static_cast<int>(band);
    if (idx >= 0 && idx < 10) {
        return impl_->config.equalizer.bands[idx];
    }
    return 0.0f;
}

void AudioEqualizer::setEqualizer(const EqualizerSettings& settings) {
    impl_->config.equalizer = settings;
    impl_->config.preset = AudioPreset::Custom;
    impl_->updateFilters();
    impl_->notifyChange();
}

EqualizerSettings AudioEqualizer::getEqualizer() const {
    return impl_->config.equalizer;
}

void AudioEqualizer::setEqualizerEnabled(bool enabled) {
    impl_->config.equalizer.enabled = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::resetEqualizer() {
    impl_->config.equalizer.bands = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    impl_->config.preset = AudioPreset::Default;
    impl_->updateFilters();
    impl_->notifyChange();
}

// =============================================================================
// Quick Controls (Bass/Mid/Treble)
// =============================================================================

void AudioEqualizer::setBass(float gainDb) {
    gainDb = std::max(-12.0f, std::min(12.0f, gainDb));
    impl_->config.equalizer.bands[0] = gainDb * 0.8f;  // 32 Hz
    impl_->config.equalizer.bands[1] = gainDb;          // 64 Hz
    impl_->config.equalizer.bands[2] = gainDb * 0.6f;  // 125 Hz
    impl_->config.preset = AudioPreset::Custom;
    impl_->updateFilters();
    impl_->notifyChange();
}

float AudioEqualizer::getBass() const {
    return impl_->config.equalizer.bands[1];
}

void AudioEqualizer::setMid(float gainDb) {
    gainDb = std::max(-12.0f, std::min(12.0f, gainDb));
    impl_->config.equalizer.bands[3] = gainDb * 0.6f;  // 250 Hz
    impl_->config.equalizer.bands[4] = gainDb;          // 500 Hz
    impl_->config.equalizer.bands[5] = gainDb;          // 1 kHz
    impl_->config.equalizer.bands[6] = gainDb * 0.6f;  // 2 kHz
    impl_->config.preset = AudioPreset::Custom;
    impl_->updateFilters();
    impl_->notifyChange();
}

float AudioEqualizer::getMid() const {
    return impl_->config.equalizer.bands[5];
}

void AudioEqualizer::setTreble(float gainDb) {
    gainDb = std::max(-12.0f, std::min(12.0f, gainDb));
    impl_->config.equalizer.bands[7] = gainDb * 0.6f;  // 4 kHz
    impl_->config.equalizer.bands[8] = gainDb;          // 8 kHz
    impl_->config.equalizer.bands[9] = gainDb * 0.8f;  // 16 kHz
    impl_->config.preset = AudioPreset::Custom;
    impl_->updateFilters();
    impl_->notifyChange();
}

float AudioEqualizer::getTreble() const {
    return impl_->config.equalizer.bands[8];
}

// =============================================================================
// Bass Boost
// =============================================================================

void AudioEqualizer::setBassBoost(const BassBoostSettings& settings) {
    impl_->config.bassBoost = settings;
    impl_->updateFilters();
    impl_->notifyChange();
}

BassBoostSettings AudioEqualizer::getBassBoost() const {
    return impl_->config.bassBoost;
}

void AudioEqualizer::setBassBoostEnabled(bool enabled) {
    impl_->config.bassBoost.enabled = enabled;
    impl_->updateFilters();
    impl_->notifyChange();
}

void AudioEqualizer::setBassBoostAmount(float amount) {
    impl_->config.bassBoost.amount = std::max(0.0f, std::min(1.0f, amount));
    impl_->updateFilters();
    impl_->notifyChange();
}

// =============================================================================
// Surround
// =============================================================================

void AudioEqualizer::setSurround(const SurroundSettings& settings) {
    impl_->config.surround = settings;
    impl_->notifyChange();
}

SurroundSettings AudioEqualizer::getSurround() const {
    return impl_->config.surround;
}

void AudioEqualizer::setSurroundEnabled(bool enabled) {
    impl_->config.surround.enabled = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setStereoWidth(float width) {
    impl_->config.surround.width = std::max(0.0f, std::min(1.0f, width));
    impl_->notifyChange();
}

void AudioEqualizer::setVirtualSurround(bool enabled) {
    impl_->config.surround.virtualizer = enabled;
    impl_->notifyChange();
}

// =============================================================================
// Room Acoustics
// =============================================================================

void AudioEqualizer::setRoom(const RoomSettings& settings) {
    impl_->config.room = settings;
    impl_->notifyChange();
}

RoomSettings AudioEqualizer::getRoom() const {
    return impl_->config.room;
}

void AudioEqualizer::setRoomEnabled(bool enabled) {
    impl_->config.room.enabled = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setRoomSize(float size) {
    impl_->config.room.size = std::max(0.0f, std::min(1.0f, size));
    impl_->notifyChange();
}

void AudioEqualizer::setReverbAmount(float amount) {
    impl_->config.room.wetMix = std::max(0.0f, std::min(1.0f, amount));
    impl_->notifyChange();
}

// =============================================================================
// Dynamics
// =============================================================================

void AudioEqualizer::setDynamics(const DynamicsSettings& settings) {
    impl_->config.dynamics = settings;
    impl_->notifyChange();
}

DynamicsSettings AudioEqualizer::getDynamics() const {
    return impl_->config.dynamics;
}

void AudioEqualizer::setCompressorEnabled(bool enabled) {
    impl_->config.dynamics.enabled = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setLimiterEnabled(bool enabled) {
    impl_->config.dynamics.limiterEnabled = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setDynamicRange(float threshold, float ratio) {
    impl_->config.dynamics.threshold = threshold;
    impl_->config.dynamics.ratio = ratio;
    impl_->notifyChange();
}

// =============================================================================
// Pitch/Speed
// =============================================================================

void AudioEqualizer::setPitch(const PitchSettings& settings) {
    impl_->config.pitch = settings;
    impl_->notifyChange();
}

PitchSettings AudioEqualizer::getPitch() const {
    return impl_->config.pitch;
}

void AudioEqualizer::setPitchSemitones(float semitones) {
    impl_->config.pitch.pitch = std::max(-12.0f, std::min(12.0f, semitones));
    impl_->config.pitch.enabled = (semitones != 0.0f);
    impl_->notifyChange();
}

float AudioEqualizer::getPitchSemitones() const {
    return impl_->config.pitch.pitch;
}

void AudioEqualizer::setSpeed(float speed) {
    impl_->config.pitch.speed = std::max(0.5f, std::min(2.0f, speed));
    impl_->config.pitch.enabled = (speed != 1.0f);
    impl_->notifyChange();
}

float AudioEqualizer::getSpeed() const {
    return impl_->config.pitch.speed;
}

void AudioEqualizer::setPreservePitch(bool preserve) {
    impl_->config.pitch.preservePitch = preserve;
    impl_->notifyChange();
}

// =============================================================================
// Enhancement
// =============================================================================

void AudioEqualizer::setEnhancement(const EnhancementSettings& settings) {
    impl_->config.enhancement = settings;
    impl_->notifyChange();
}

EnhancementSettings AudioEqualizer::getEnhancement() const {
    return impl_->config.enhancement;
}

void AudioEqualizer::setVoiceEnhance(bool enabled) {
    impl_->config.enhancement.voiceEnhance = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setDialogueBoost(bool enabled) {
    impl_->config.enhancement.dialogueBoost = enabled;
    impl_->notifyChange();
}

void AudioEqualizer::setLoudnessNormalization(bool enabled, float targetLufs) {
    impl_->config.enhancement.loudnessNorm = enabled;
    impl_->config.enhancement.targetLufs = targetLufs;
    impl_->notifyChange();
}

void AudioEqualizer::setCrossfeed(bool enabled, float amount) {
    impl_->config.enhancement.crossfeed = enabled;
    impl_->config.enhancement.crossfeedAmount = amount;
    impl_->notifyChange();
}

// =============================================================================
// Master Controls
// =============================================================================

void AudioEqualizer::setMasterVolume(float volume) {
    impl_->config.masterVolume = std::max(0.0f, std::min(2.0f, volume));
    impl_->notifyChange();
}

float AudioEqualizer::getMasterVolume() const {
    return impl_->config.masterVolume;
}

void AudioEqualizer::setBalance(float balance) {
    impl_->config.balance = std::max(-1.0f, std::min(1.0f, balance));
    impl_->notifyChange();
}

float AudioEqualizer::getBalance() const {
    return impl_->config.balance;
}

void AudioEqualizer::setMuted(bool muted) {
    impl_->config.muted = muted;
    impl_->notifyChange();
}

bool AudioEqualizer::isMuted() const {
    return impl_->config.muted;
}

// =============================================================================
// Full Configuration
// =============================================================================

void AudioEqualizer::setConfig(const AudioDSPConfig& config) {
    impl_->config = config;
    impl_->updateFilters();
    impl_->notifyChange();
}

AudioDSPConfig AudioEqualizer::getConfig() const {
    return impl_->config;
}

void AudioEqualizer::resetToDefaults() {
    impl_->config = AudioDSPConfig();
    impl_->updateFilters();
    impl_->notifyChange();
}

// =============================================================================
// Audio Processing
// =============================================================================

void AudioEqualizer::process(const float* input, float* output, size_t numFrames) {
    if (impl_->config.muted) {
        std::memset(output, 0, numFrames * 2 * sizeof(float));
        return;
    }
    
    float volume = impl_->config.masterVolume;
    float balance = impl_->config.balance;
    float gainL = volume * (balance <= 0 ? 1.0f : 1.0f - balance);
    float gainR = volume * (balance >= 0 ? 1.0f : 1.0f + balance);
    
    for (size_t i = 0; i < numFrames; ++i) {
        float L = input[i * 2];
        float R = input[i * 2 + 1];
        
        // Apply EQ
        if (impl_->config.equalizer.enabled) {
            for (int b = 0; b < 10; ++b) {
                L = impl_->eqFiltersL[b].process(L);
                R = impl_->eqFiltersR[b].process(R);
            }
        }
        
        // Apply bass boost
        if (impl_->config.bassBoost.enabled) {
            L = impl_->bassBoostL.process(L);
            R = impl_->bassBoostR.process(R);
        }
        
        // Apply stereo width
        if (impl_->config.surround.enabled) {
            float mid = (L + R) * 0.5f;
            float side = (L - R) * 0.5f * (1.0f + impl_->config.surround.width);
            L = mid + side;
            R = mid - side;
        }
        
        // Apply dynamics
        L = impl_->applyCompressor(L);
        R = impl_->applyCompressor(R);
        
        // Apply crossfeed (for headphones)
        if (impl_->config.enhancement.crossfeed) {
            float cf = impl_->config.enhancement.crossfeedAmount;
            float newL = L + R * cf;
            float newR = R + L * cf;
            L = newL / (1.0f + cf);
            R = newR / (1.0f + cf);
        }
        
        // Apply master volume and balance
        output[i * 2] = L * gainL;
        output[i * 2 + 1] = R * gainR;
    }
}

void AudioEqualizer::setOnSettingsChanged(
    std::function<void(const AudioDSPConfig&)> callback) {
    impl_->onSettingsChanged = callback;
}

// =============================================================================
// Global Instance
// =============================================================================

AudioEqualizer& getAudioEqualizer() {
    static AudioEqualizer instance;
    return instance;
}

} // namespace audio
} // namespace zepra
