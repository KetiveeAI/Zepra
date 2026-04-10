// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "media_element.h"
#include <algorithm>
#include <cmath>

namespace NXRender {
namespace Media {

// ==================================================================
// TextTrack
// ==================================================================

const TextTrackCue* TextTrack::activeCue(double time) const {
    for (const auto& cue : cues) {
        if (time >= cue.startTime && time < cue.endTime) {
            return &cue;
        }
    }
    return nullptr;
}

std::vector<const TextTrackCue*> TextTrack::activeCues(double time) const {
    std::vector<const TextTrackCue*> result;
    for (const auto& cue : cues) {
        if (time >= cue.startTime && time < cue.endTime) {
            result.push_back(&cue);
        }
    }
    return result;
}

// ==================================================================
// MediaElement
// ==================================================================

MediaElement::MediaElement() {}
MediaElement::~MediaElement() {}

void MediaElement::setSrc(const std::string& url) {
    std::lock_guard<std::mutex> lock(mutex_);
    src_ = url;
    state_ = MediaState::Empty;
    currentTime_ = 0;
    duration_ = 0;
    error_ = MediaError();
}

void MediaElement::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = MediaState::Loading;
    networkState_ = NetworkState::Loading;
    fireEvent(MediaEvent::LoadStart);
}

void MediaElement::play() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == MediaState::Error) return;
    if (state_ == MediaState::Ended && loop_) {
        currentTime_ = 0;
    }
    state_ = MediaState::Playing;
    fireEvent(MediaEvent::Play);
    fireEvent(MediaEvent::Playing);
}

void MediaElement::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != MediaState::Playing) return;
    state_ = MediaState::Paused;
    fireEvent(MediaEvent::Pause);
}

void MediaElement::setCurrentTime(double time) {
    std::lock_guard<std::mutex> lock(mutex_);
    seeking_ = true;
    fireEvent(MediaEvent::Seeking);

    currentTime_ = std::clamp(time, 0.0, duration_);

    // Add to played ranges
    if (!played_.ranges.empty()) {
        auto& last = played_.ranges.back();
        if (currentTime_ < last.end) {
            // Seeking backward — start new range
            played_.ranges.push_back({currentTime_, currentTime_});
        }
    } else {
        played_.ranges.push_back({currentTime_, currentTime_});
    }

    seeking_ = false;
    fireEvent(MediaEvent::Seeked);
    fireEvent(MediaEvent::TimeUpdate);
}

TimeRanges MediaElement::seekable() const {
    TimeRanges result;
    if (duration_ > 0) {
        result.ranges.push_back({0, duration_});
    }
    return result;
}

void MediaElement::setVolume(float vol) {
    volume_ = std::clamp(vol, 0.0f, 1.0f);
    fireEvent(MediaEvent::VolumeChange);
}

void MediaElement::setMuted(bool m) {
    muted_ = m;
    fireEvent(MediaEvent::VolumeChange);
}

void MediaElement::setPlaybackRate(double rate) {
    playbackRate_ = std::clamp(rate, 0.0625, 16.0);
    fireEvent(MediaEvent::RateChange);
}

void MediaElement::addTextTrack(const TextTrack& track) {
    textTracks_.push_back(track);
}

void MediaElement::setActiveTextTrack(int index) {
    for (size_t i = 0; i < textTracks_.size(); i++) {
        textTracks_[i].active = (static_cast<int>(i) == index);
    }
}

void MediaElement::addEventListener(MediaEvent event, MediaEventCallback callback) {
    eventListeners_[event].push_back(callback);
}

void MediaElement::fireEvent(MediaEvent event) {
    auto it = eventListeners_.find(event);
    if (it != eventListeners_.end()) {
        for (auto& cb : it->second) {
            cb(event);
        }
    }
}

// ==================================================================
// GainNode
// ==================================================================

void GainNode::process(AudioFrame& frame) {
    for (auto& sample : frame.samples) {
        sample *= gain_;
    }
}

// ==================================================================
// BiquadFilterNode
// ==================================================================

void BiquadFilterNode::process(AudioFrame& frame) {
    // Simplified first-order IIR approximation
    // Real implementation would use proper biquad coefficients
    float cutoff = frequency_ / 44100.0f;
    float rc = 1.0f / (cutoff * 2 * 3.14159f);
    float dt = 1.0f / 44100.0f;
    float alpha = dt / (rc + dt);

    if (type_ == FilterType::LowPass) {
        for (int ch = 0; ch < frame.channels; ch++) {
            float prev = 0;
            for (int i = ch; i < static_cast<int>(frame.samples.size()); i += frame.channels) {
                frame.samples[i] = prev + alpha * (frame.samples[i] - prev);
                prev = frame.samples[i];
            }
        }
    } else if (type_ == FilterType::HighPass) {
        for (int ch = 0; ch < frame.channels; ch++) {
            float prev = 0, prevRaw = 0;
            for (int i = ch; i < static_cast<int>(frame.samples.size()); i += frame.channels) {
                float raw = frame.samples[i];
                frame.samples[i] = alpha * (prev + raw - prevRaw);
                prev = frame.samples[i];
                prevRaw = raw;
            }
        }
    }
    (void)q_;
    (void)gain_;
}

// ==================================================================
// OscillatorNode
// ==================================================================

void OscillatorNode::start(double /*when*/) {
    running_ = true;
    phase_ = 0;
}

void OscillatorNode::stop(double /*when*/) {
    running_ = false;
}

void OscillatorNode::process(AudioFrame& frame) {
    if (!running_) return;

    float freq = frequency_ * std::pow(2.0f, detune_ / 1200.0f);
    double phaseInc = freq / 44100.0;

    for (int i = 0; i < frame.sampleCount; i++) {
        float sample = 0;
        switch (type_) {
            case WaveType::Sine:
                sample = std::sin(phase_ * 2 * M_PI);
                break;
            case WaveType::Square:
                sample = (std::fmod(phase_, 1.0) < 0.5) ? 1.0f : -1.0f;
                break;
            case WaveType::Sawtooth:
                sample = 2.0f * static_cast<float>(std::fmod(phase_, 1.0)) - 1.0f;
                break;
            case WaveType::Triangle: {
                double t = std::fmod(phase_, 1.0);
                sample = (t < 0.5) ? (4.0f * static_cast<float>(t) - 1.0f) :
                                      (3.0f - 4.0f * static_cast<float>(t));
                break;
            }
            case WaveType::Custom:
                sample = 0;
                break;
        }

        for (int ch = 0; ch < frame.channels; ch++) {
            frame.samples[i * frame.channels + ch] = sample;
        }
        phase_ += phaseInc;
    }
}

// ==================================================================
// AudioContext
// ==================================================================

AudioContext::AudioContext() {}
AudioContext::~AudioContext() { close(); }

void AudioContext::resume() { state_ = "running"; }
void AudioContext::suspend() { state_ = "suspended"; }
void AudioContext::close() { state_ = "closed"; }

std::unique_ptr<GainNode> AudioContext::createGain() {
    return std::make_unique<GainNode>();
}

std::unique_ptr<BiquadFilterNode> AudioContext::createBiquadFilter() {
    return std::make_unique<BiquadFilterNode>();
}

std::unique_ptr<OscillatorNode> AudioContext::createOscillator() {
    return std::make_unique<OscillatorNode>();
}

void AudioContext::decodeAudioData(const std::vector<uint8_t>& /*data*/,
                                     std::function<void(AudioFrame)> /*onSuccess*/,
                                     std::function<void(std::string)> onError) {
    // Bridge point — actual decoding handled by platform audio decoder
    if (onError) onError("decodeAudioData: not connected to platform decoder");
}

void AudioContext::DestinationNode::process(AudioFrame& /*frame*/) {
    // Output to platform audio backend
}

} // namespace Media
} // namespace NXRender
