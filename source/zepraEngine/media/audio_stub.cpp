// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file audio_stub.cpp
 * @brief Minimal stub implementations for AudioEqualizer and BrowserAudio
 * @note Real implementations exist in audio_equalizer.cpp and browser_audio.cpp
 *       but have external dependencies. These stubs allow build without those deps.
 */

#include "engine/audio_equalizer.h"
#include "engine/browser_audio.h"

namespace zepra {
namespace audio {

// Impl classes for pimpl pattern (required for unique_ptr destructor)
class AudioEqualizer::Impl {
public:
    AudioDSPConfig config;
};

class BrowserAudio::Impl {
public:
    bool initialized = false;
    float masterVolume = 1.0f;
    bool muted = false;
    AudioQuality quality = AudioQuality::Balanced;
    PowerMode powerMode = PowerMode::Balanced;
    SpatialAudioConfig spatialConfig;
};

// =============================================================================
// AudioEqualizer Stubs
// =============================================================================

AudioEqualizer::AudioEqualizer() : impl_(std::make_unique<Impl>()) {}
AudioEqualizer::~AudioEqualizer() = default;

void AudioEqualizer::applyPreset(AudioPreset preset) {}
AudioPreset AudioEqualizer::getCurrentPreset() const { return AudioPreset::Default; }
EqualizerSettings AudioEqualizer::getPresetEQ(AudioPreset preset) const { return EqualizerSettings(); }
bool AudioEqualizer::saveCustomPreset(const std::string& name) { return true; }
bool AudioEqualizer::loadCustomPreset(const std::string& name) { return false; }
std::vector<std::string> AudioEqualizer::getCustomPresets() const { return {}; }

void AudioEqualizer::setBand(EQBand band, float gainDb) {}
float AudioEqualizer::getBand(EQBand band) const { return 0.0f; }
void AudioEqualizer::setEqualizer(const EqualizerSettings& settings) {}
EqualizerSettings AudioEqualizer::getEqualizer() const { return EqualizerSettings(); }
void AudioEqualizer::setEqualizerEnabled(bool enabled) {}
void AudioEqualizer::resetEqualizer() {}

void AudioEqualizer::setBass(float gainDb) {}
float AudioEqualizer::getBass() const { return 0.0f; }
void AudioEqualizer::setMid(float gainDb) {}
float AudioEqualizer::getMid() const { return 0.0f; }
void AudioEqualizer::setTreble(float gainDb) {}
float AudioEqualizer::getTreble() const { return 0.0f; }

void AudioEqualizer::setBassBoost(const BassBoostSettings& settings) {}
BassBoostSettings AudioEqualizer::getBassBoost() const { return BassBoostSettings(); }
void AudioEqualizer::setBassBoostEnabled(bool enabled) {}
void AudioEqualizer::setBassBoostAmount(float amount) {}

void AudioEqualizer::setSurround(const SurroundSettings& settings) {}
SurroundSettings AudioEqualizer::getSurround() const { return SurroundSettings(); }
void AudioEqualizer::setSurroundEnabled(bool enabled) {}
void AudioEqualizer::setStereoWidth(float width) {}
void AudioEqualizer::setVirtualSurround(bool enabled) {}

void AudioEqualizer::setRoom(const RoomSettings& settings) {}
RoomSettings AudioEqualizer::getRoom() const { return RoomSettings(); }
void AudioEqualizer::setRoomEnabled(bool enabled) {}
void AudioEqualizer::setRoomSize(float size) {}
void AudioEqualizer::setReverbAmount(float amount) {}

void AudioEqualizer::setDynamics(const DynamicsSettings& settings) {}
DynamicsSettings AudioEqualizer::getDynamics() const { return DynamicsSettings(); }
void AudioEqualizer::setCompressorEnabled(bool enabled) {}
void AudioEqualizer::setLimiterEnabled(bool enabled) {}
void AudioEqualizer::setDynamicRange(float threshold, float ratio) {}

void AudioEqualizer::setPitch(const PitchSettings& settings) {}
PitchSettings AudioEqualizer::getPitch() const { return PitchSettings(); }
void AudioEqualizer::setPitchSemitones(float semitones) {}
float AudioEqualizer::getPitchSemitones() const { return 0.0f; }
void AudioEqualizer::setSpeed(float speed) {}
float AudioEqualizer::getSpeed() const { return 1.0f; }
void AudioEqualizer::setPreservePitch(bool preserve) {}

void AudioEqualizer::setEnhancement(const EnhancementSettings& settings) {}
EnhancementSettings AudioEqualizer::getEnhancement() const { return EnhancementSettings(); }
void AudioEqualizer::setVoiceEnhance(bool enabled) {}
void AudioEqualizer::setDialogueBoost(bool enabled) {}
void AudioEqualizer::setLoudnessNormalization(bool enabled, float targetLufs) {}
void AudioEqualizer::setCrossfeed(bool enabled, float amount) {}

void AudioEqualizer::setMasterVolume(float volume) {}
float AudioEqualizer::getMasterVolume() const { return 1.0f; }
void AudioEqualizer::setBalance(float balance) {}
float AudioEqualizer::getBalance() const { return 0.0f; }
void AudioEqualizer::setMuted(bool muted) {}
bool AudioEqualizer::isMuted() const { return false; }

void AudioEqualizer::setConfig(const AudioDSPConfig& config) {}
AudioDSPConfig AudioEqualizer::getConfig() const { return AudioDSPConfig(); }
void AudioEqualizer::resetToDefaults() {}
void AudioEqualizer::process(const float* input, float* output, size_t numFrames) {}
void AudioEqualizer::setOnSettingsChanged(std::function<void(const AudioDSPConfig&)> callback) {}

AudioEqualizer& getAudioEqualizer() {
    static AudioEqualizer instance;
    return instance;
}

std::string getPresetName(AudioPreset preset) { return "Default"; }
std::vector<AudioPreset> getAllPresets() { return { AudioPreset::Default }; }
float getBandFrequency(EQBand band) { return 1000.0f; }
std::string getBandName(EQBand band) { return "1 kHz"; }

// =============================================================================
// BrowserAudio Stubs  
// =============================================================================

BrowserAudio::BrowserAudio() : impl_(std::make_unique<Impl>()) {}
BrowserAudio::~BrowserAudio() = default;

bool BrowserAudio::initialize() { return true; }
void BrowserAudio::shutdown() {}
bool BrowserAudio::isInitialized() const { return true; }

std::vector<AudioDevice> BrowserAudio::getOutputDevices() const { return { {"default", "Default", true, false, 2, 48000} }; }
std::vector<AudioDevice> BrowserAudio::getInputDevices() const { return {}; }
bool BrowserAudio::setOutputDevice(const std::string& deviceId) { return true; }
bool BrowserAudio::setInputDevice(const std::string& deviceId) { return true; }
std::string BrowserAudio::getCurrentOutputDevice() const { return "default"; }

float BrowserAudio::getMasterVolume() const { return 1.0f; }
void BrowserAudio::setMasterVolume(float volume) {}
bool BrowserAudio::isMuted() const { return false; }
void BrowserAudio::setMuted(bool muted) {}

AudioQuality BrowserAudio::getQuality() const { return AudioQuality::Balanced; }
void BrowserAudio::setQuality(AudioQuality quality) {}
SpatialAudioConfig BrowserAudio::getSpatialConfig() const { return SpatialAudioConfig(); }
void BrowserAudio::setSpatialConfig(const SpatialAudioConfig& config) {}
bool BrowserAudio::isSpatialAudioSupported() const { return false; }
PowerMode BrowserAudio::getPowerMode() const { return PowerMode::Balanced; }
void BrowserAudio::setPowerMode(PowerMode mode) {}

void BrowserAudio::muteTab(int tabId) {}
void BrowserAudio::unmuteTab(int tabId) {}
bool BrowserAudio::isTabMuted(int tabId) const { return false; }
float BrowserAudio::getTabVolume(int tabId) const { return 1.0f; }
void BrowserAudio::setTabVolume(int tabId, float volume) {}

AudioStats BrowserAudio::getStats() const { return AudioStats(); }
bool BrowserAudio::hasAVX2Support() const { return false; }
bool BrowserAudio::hasNeonSupport() const { return false; }
bool BrowserAudio::isArmPlatform() const { return false; }

void BrowserAudio::setOnDeviceChange(std::function<void()> callback) {}
void BrowserAudio::setOnVolumeChange(std::function<void(float)> callback) {}

std::string BrowserAudio::detectCodec(const uint8_t* data, size_t size) const { return "Unknown"; }
bool BrowserAudio::isDolbyStream(const uint8_t* data, size_t size) const { return false; }
bool BrowserAudio::isDTSStream(const uint8_t* data, size_t size) const { return false; }
bool BrowserAudio::canPassthrough(const std::string& codec) const { return false; }
bool BrowserAudio::isNeolyxOS() const { return false; }
void BrowserAudio::enableHardwareDecode(bool enabled) {}
void BrowserAudio::enableNeolyxSpatial(bool enabled) {}

BrowserAudio& getBrowserAudio() {
    static BrowserAudio instance;
    return instance;
}

} // namespace audio
} // namespace zepra
