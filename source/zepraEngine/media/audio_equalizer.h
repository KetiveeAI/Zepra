// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_AUDIO_EQUALIZER_H
#define ZEPRA_AUDIO_EQUALIZER_H

/**
 * @file audio_equalizer.h
 * @brief Advanced audio equalizer and DSP controls
 * 
 * Features:
 * - One-click preset modes (Gaming, Music, Movie, etc.)
 * - Full 10-band equalizer
 * - Bass boost, treble, surround
 * - Pitch and speed control
 * - Room acoustics simulation
 * - Compressor/limiter
 * - Full manual customization
 */

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <cstdint>

namespace zepra {
namespace audio {

// =============================================================================
// Audio Presets (One-Click Modes)
// =============================================================================

/**
 * @brief One-click audio environment presets
 */
enum class AudioPreset {
    Default,        // Flat, no processing
    
    // Content-based
    Music,          // Balanced audio for music
    Movie,          // Cinema-like with surround
    Gaming,         // Enhanced footsteps, directional audio
    Podcast,        // Voice-optimized
    Audiobook,      // Clear voice, reduced fatigue
    
    // Genre-specific
    Pop,            // Bright, punchy
    Rock,           // Strong mids, bass
    Classical,      // Wide dynamic range
    Jazz,           // Warm, smooth
    EDM,            // Heavy bass, crisp highs
    HipHop,         // Deep bass, clear vocals
    
    // Environment
    LateNight,      // Compressed dynamic range
    LoudSpeaker,    // For speakers
    Headphones,     // Optimized for headphones
    SmallSpeakers,  // Compensate for small speakers
    
    // Custom
    Custom          // User-defined
};

/**
 * @brief Get preset name
 */
std::string getPresetName(AudioPreset preset);

/**
 * @brief Get all available presets
 */
std::vector<AudioPreset> getAllPresets();

// =============================================================================
// Equalizer Bands
// =============================================================================

/**
 * @brief 10-band equalizer frequencies
 */
enum class EQBand {
    Band32Hz = 0,   // Sub-bass
    Band64Hz,       // Bass
    Band125Hz,      // Low-mid bass
    Band250Hz,      // Low-mid
    Band500Hz,      // Mid
    Band1kHz,       // Mid
    Band2kHz,       // Upper-mid
    Band4kHz,       // Presence
    Band8kHz,       // Brilliance
    Band16kHz,      // Air
    NumBands
};

/**
 * @brief Get band frequency in Hz
 */
float getBandFrequency(EQBand band);

/**
 * @brief Get band name
 */
std::string getBandName(EQBand band);

/**
 * @brief Equalizer settings (10-band)
 * Gain values: -12.0 to +12.0 dB
 */
struct EqualizerSettings {
    std::array<float, 10> bands = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool enabled = true;
    
    // Quick access
    float& bass()       { return bands[1]; }  // 64Hz
    float& lowMid()     { return bands[3]; }  // 250Hz
    float& mid()        { return bands[5]; }  // 1kHz
    float& highMid()    { return bands[7]; }  // 4kHz
    float& treble()     { return bands[9]; }  // 16kHz
    
    const float& bass()     const { return bands[1]; }
    const float& lowMid()   const { return bands[3]; }
    const float& mid()      const { return bands[5]; }
    const float& highMid()  const { return bands[7]; }
    const float& treble()   const { return bands[9]; }
};

// =============================================================================
// Advanced DSP Controls
// =============================================================================

/**
 * @brief Bass boost settings
 */
struct BassBoostSettings {
    bool enabled = false;
    float amount = 0.0f;        // 0.0 - 1.0 (0% - 100%)
    float frequency = 100.0f;   // Cutoff frequency (Hz)
};

/**
 * @brief Virtual surround settings
 */
struct SurroundSettings {
    bool enabled = false;
    float width = 0.5f;         // Stereo width (0.0 - 1.0)
    float depth = 0.5f;         // Front/back depth
    bool virtualizer = false;   // Virtual surround from stereo
};

/**
 * @brief Room acoustics simulation
 */
struct RoomSettings {
    bool enabled = false;
    float size = 0.3f;          // Room size (0.0 - 1.0)
    float damping = 0.5f;       // High-frequency damping
    float wetMix = 0.2f;        // Effect amount (0.0 - 1.0)
    float decay = 0.5f;         // Reverb decay time
};

/**
 * @brief Dynamics processing (compressor/limiter)
 */
struct DynamicsSettings {
    bool enabled = false;
    
    // Compressor
    float threshold = -20.0f;   // dB
    float ratio = 4.0f;         // 1:1 to 20:1
    float attack = 10.0f;       // ms
    float release = 100.0f;     // ms
    float makeupGain = 0.0f;    // dB
    
    // Limiter
    bool limiterEnabled = true;
    float limiterCeiling = -0.3f; // dB
};

/**
 * @brief Pitch and speed control
 */
struct PitchSettings {
    bool enabled = false;
    float pitch = 0.0f;         // Semitones (-12 to +12)
    float speed = 1.0f;         // Playback speed (0.5 - 2.0)
    bool preservePitch = true;  // Keep pitch when changing speed
};

/**
 * @brief Audio enhancement settings
 */
struct EnhancementSettings {
    // Clarity
    bool voiceEnhance = false;  // Boost voice frequencies
    bool dialogueBoost = false; // Enhance movie dialogue
    float clarity = 0.0f;       // -1.0 to 1.0
    
    // Loudness
    bool loudnessNorm = false;  // Normalize loudness across tracks
    float targetLufs = -14.0f;  // Target loudness (LUFS)
    
    // Spatial
    bool crossfeed = false;     // Reduce fatigue for headphones
    float crossfeedAmount = 0.3f;
};

// =============================================================================
// Complete Audio DSP Configuration
// =============================================================================

/**
 * @brief Complete audio processing configuration
 */
struct AudioDSPConfig {
    // Master
    float masterVolume = 1.0f;
    float balance = 0.0f;       // Left/Right balance (-1.0 to 1.0)
    bool muted = false;
    
    // Preset
    AudioPreset preset = AudioPreset::Default;
    
    // Equalizer
    EqualizerSettings equalizer;
    
    // DSP
    BassBoostSettings bassBoost;
    SurroundSettings surround;
    RoomSettings room;
    DynamicsSettings dynamics;
    PitchSettings pitch;
    EnhancementSettings enhancement;
    
    // Output
    int outputSampleRate = 48000;
    int outputBitDepth = 16;
    int outputChannels = 2;
};

// =============================================================================
// Audio Equalizer Class
// =============================================================================

/**
 * @brief Full-featured audio equalizer and DSP processor
 */
class AudioEqualizer {
public:
    AudioEqualizer();
    ~AudioEqualizer();
    
    // ==========================================================================
    // Presets (One-Click)
    // ==========================================================================
    
    /**
     * @brief Apply a preset with one click
     */
    void applyPreset(AudioPreset preset);
    
    /**
     * @brief Get current active preset
     */
    AudioPreset getCurrentPreset() const;
    
    /**
     * @brief Get preset's EQ settings
     */
    EqualizerSettings getPresetEQ(AudioPreset preset) const;
    
    /**
     * @brief Save current settings as custom preset
     */
    bool saveCustomPreset(const std::string& name);
    
    /**
     * @brief Load custom preset by name
     */
    bool loadCustomPreset(const std::string& name);
    
    /**
     * @brief Get list of custom presets
     */
    std::vector<std::string> getCustomPresets() const;
    
    // ==========================================================================
    // Equalizer Control
    // ==========================================================================
    
    /**
     * @brief Set individual EQ band
     * @param band The frequency band
     * @param gainDb Gain in dB (-12 to +12)
     */
    void setBand(EQBand band, float gainDb);
    
    /**
     * @brief Get individual EQ band
     */
    float getBand(EQBand band) const;
    
    /**
     * @brief Set all EQ bands at once
     */
    void setEqualizer(const EqualizerSettings& settings);
    
    /**
     * @brief Get current EQ settings
     */
    EqualizerSettings getEqualizer() const;
    
    /**
     * @brief Enable/disable equalizer
     */
    void setEqualizerEnabled(bool enabled);
    
    /**
     * @brief Reset equalizer to flat
     */
    void resetEqualizer();
    
    // ==========================================================================
    // Quick Controls (Bass/Mid/Treble)
    // ==========================================================================
    
    /**
     * @brief Set bass level (-12 to +12 dB)
     */
    void setBass(float gainDb);
    float getBass() const;
    
    /**
     * @brief Set mid level (-12 to +12 dB)
     */
    void setMid(float gainDb);
    float getMid() const;
    
    /**
     * @brief Set treble level (-12 to +12 dB)
     */
    void setTreble(float gainDb);
    float getTreble() const;
    
    // ==========================================================================
    // Bass Boost
    // ==========================================================================
    
    void setBassBoost(const BassBoostSettings& settings);
    BassBoostSettings getBassBoost() const;
    void setBassBoostEnabled(bool enabled);
    void setBassBoostAmount(float amount);
    
    // ==========================================================================
    // Surround/Spatial
    // ==========================================================================
    
    void setSurround(const SurroundSettings& settings);
    SurroundSettings getSurround() const;
    void setSurroundEnabled(bool enabled);
    void setStereoWidth(float width);
    void setVirtualSurround(bool enabled);
    
    // ==========================================================================
    // Room Acoustics
    // ==========================================================================
    
    void setRoom(const RoomSettings& settings);
    RoomSettings getRoom() const;
    void setRoomEnabled(bool enabled);
    void setRoomSize(float size);
    void setReverbAmount(float amount);
    
    // ==========================================================================
    // Dynamics (Compressor/Limiter)
    // ==========================================================================
    
    void setDynamics(const DynamicsSettings& settings);
    DynamicsSettings getDynamics() const;
    void setCompressorEnabled(bool enabled);
    void setLimiterEnabled(bool enabled);
    void setDynamicRange(float threshold, float ratio);
    
    // ==========================================================================
    // Pitch/Speed Control
    // ==========================================================================
    
    void setPitch(const PitchSettings& settings);
    PitchSettings getPitch() const;
    
    /**
     * @brief Set pitch in semitones
     * @param semitones -12 to +12 semitones
     */
    void setPitchSemitones(float semitones);
    float getPitchSemitones() const;
    
    /**
     * @brief Set playback speed
     * @param speed 0.5x to 2.0x
     */
    void setSpeed(float speed);
    float getSpeed() const;
    
    /**
     * @brief Preserve pitch when changing speed
     */
    void setPreservePitch(bool preserve);
    
    // ==========================================================================
    // Enhancement
    // ==========================================================================
    
    void setEnhancement(const EnhancementSettings& settings);
    EnhancementSettings getEnhancement() const;
    void setVoiceEnhance(bool enabled);
    void setDialogueBoost(bool enabled);
    void setLoudnessNormalization(bool enabled, float targetLufs = -14.0f);
    void setCrossfeed(bool enabled, float amount = 0.3f);
    
    // ==========================================================================
    // Master Controls
    // ==========================================================================
    
    void setMasterVolume(float volume);
    float getMasterVolume() const;
    
    void setBalance(float balance);
    float getBalance() const;
    
    void setMuted(bool muted);
    bool isMuted() const;
    
    // ==========================================================================
    // Full Configuration
    // ==========================================================================
    
    void setConfig(const AudioDSPConfig& config);
    AudioDSPConfig getConfig() const;
    void resetToDefaults();
    
    // ==========================================================================
    // Audio Processing
    // ==========================================================================
    
    /**
     * @brief Process audio samples
     * @param input Input samples (interleaved stereo float)
     * @param output Output buffer
     * @param numFrames Number of frames
     */
    void process(const float* input, float* output, size_t numFrames);
    
    /**
     * @brief Callback when settings change
     */
    void setOnSettingsChanged(std::function<void(const AudioDSPConfig&)> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Global audio equalizer instance
 */
AudioEqualizer& getAudioEqualizer();

} // namespace audio
} // namespace zepra

#endif // ZEPRA_AUDIO_EQUALIZER_H
