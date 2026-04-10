// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

namespace NXRender {
namespace Media {

// ==================================================================
// Media types and formats
// ==================================================================

enum class MediaType : uint8_t {
    Audio,
    Video,
};

enum class MediaState : uint8_t {
    Empty,      // No source loaded
    Loading,    // Source is loading
    Ready,      // Ready to play
    Playing,
    Paused,
    Ended,
    Error,
};

enum class NetworkState : uint8_t {
    Empty,
    Idle,
    Loading,
    NoSource,
};

struct MediaError {
    enum Code {
        None = 0,
        Aborted = 1,
        Network = 2,
        Decode = 3,
        SrcNotSupported = 4,
    } code = None;
    std::string message;
};

struct TimeRange {
    double start = 0;
    double end = 0;
};

struct TimeRanges {
    std::vector<TimeRange> ranges;
    size_t length() const { return ranges.size(); }
    double start(size_t index) const { return index < ranges.size() ? ranges[index].start : 0; }
    double end(size_t index) const { return index < ranges.size() ? ranges[index].end : 0; }
};

// ==================================================================
// Audio buffer / frame
// ==================================================================

struct AudioFormat {
    int sampleRate = 44100;
    int channels = 2;
    int bitsPerSample = 16;
    bool floatSamples = false;
};

struct AudioFrame {
    std::vector<float> samples;
    int channels = 2;
    int sampleCount = 0;
    double timestamp = 0;
};

// ==================================================================
// Video frame
// ==================================================================

struct VideoFormat {
    int width = 0, height = 0;
    float frameRate = 30;
    float aspectRatio = 0; // 0 = derive from width/height
    enum class PixelFormat {
        RGBA, BGRA, YUV420, NV12, NV21
    } pixelFormat = PixelFormat::RGBA;
};

struct VideoFrame {
    std::vector<uint8_t> data;
    int width = 0, height = 0;
    VideoFormat::PixelFormat format = VideoFormat::PixelFormat::RGBA;
    double timestamp = 0;
    double duration = 0;
    bool keyFrame = false;
    uint32_t textureId = 0; // GPU texture if uploaded
};

// ==================================================================
// Media source
// ==================================================================

struct MediaSource {
    std::string url;
    std::string mimeType;
    double duration = 0;
    AudioFormat audioFormat;
    VideoFormat videoFormat;
    bool hasAudio = false;
    bool hasVideo = false;
};

// ==================================================================
// Media events
// ==================================================================

enum class MediaEvent : uint8_t {
    LoadStart,
    Progress,
    Suspend,
    Abort,
    Error,
    Emptied,
    Stalled,
    LoadedMetadata,
    LoadedData,
    CanPlay,
    CanPlayThrough,
    Playing,
    Waiting,
    Seeking,
    Seeked,
    Ended,
    DurationChange,
    TimeUpdate,
    Play,
    Pause,
    RateChange,
    VolumeChange,
    Resize,
};

using MediaEventCallback = std::function<void(MediaEvent)>;

// ==================================================================
// Text track (subtitles/captions)
// ==================================================================

struct TextTrackCue {
    double startTime = 0;
    double endTime = 0;
    std::string text;
    std::string id;
    std::string align = "center"; // start, center, end
    int line = -1;
    int position = 50;
    int size = 100;
    bool vertical = false;
    bool snapToLines = true;
};

struct TextTrack {
    std::string kind = "subtitles"; // subtitles, captions, descriptions, chapters, metadata
    std::string label;
    std::string language;
    std::string src;
    bool active = false;
    std::vector<TextTrackCue> cues;

    const TextTrackCue* activeCue(double time) const;
    std::vector<const TextTrackCue*> activeCues(double time) const;
};

// ==================================================================
// Media element (HTML5 media API)
// ==================================================================

class MediaElement {
public:
    MediaElement();
    virtual ~MediaElement();

    // Source
    void setSrc(const std::string& url);
    const std::string& src() const { return src_; }
    void load();

    // Playback
    void play();
    void pause();
    bool paused() const { return state_ == MediaState::Paused || state_ == MediaState::Ready; }
    bool ended() const { return state_ == MediaState::Ended; }
    MediaState readyState() const { return state_; }

    // Time
    double currentTime() const { return currentTime_; }
    void setCurrentTime(double time);
    double duration() const { return duration_; }
    TimeRanges buffered() const { return buffered_; }
    TimeRanges seekable() const;
    TimeRanges played() const { return played_; }
    bool seeking() const { return seeking_; }

    // Volume
    float volume() const { return volume_; }
    void setVolume(float vol);
    bool muted() const { return muted_; }
    void setMuted(bool m);

    // Playback rate
    double playbackRate() const { return playbackRate_; }
    void setPlaybackRate(double rate);
    double defaultPlaybackRate() const { return defaultPlaybackRate_; }

    // Loop
    bool loop() const { return loop_; }
    void setLoop(bool l) { loop_ = l; }

    // Preload
    std::string preload() const { return preload_; }
    void setPreload(const std::string& p) { preload_ = p; }

    // Autoplay
    bool autoplay() const { return autoplay_; }
    void setAutoplay(bool a) { autoplay_ = a; }

    // Controls
    bool controls() const { return controls_; }
    void setControls(bool c) { controls_ = c; }

    // Cross-origin
    std::string crossOrigin() const { return crossOrigin_; }
    void setCrossOrigin(const std::string& co) { crossOrigin_ = co; }

    // Error
    MediaError error() const { return error_; }
    NetworkState networkState() const { return networkState_; }

    // Text tracks
    void addTextTrack(const TextTrack& track);
    const std::vector<TextTrack>& textTracks() const { return textTracks_; }
    void setActiveTextTrack(int index);

    // Event handling
    void addEventListener(MediaEvent event, MediaEventCallback callback);

    // Frame access (for video)
    virtual VideoFrame* currentFrame() { return nullptr; }
    virtual void uploadFrameToTexture() {}

protected:
    std::string src_;
    MediaState state_ = MediaState::Empty;
    NetworkState networkState_ = NetworkState::Empty;
    MediaError error_;
    double currentTime_ = 0;
    double duration_ = 0;
    float volume_ = 1.0f;
    bool muted_ = false;
    double playbackRate_ = 1.0;
    double defaultPlaybackRate_ = 1.0;
    bool loop_ = false;
    bool autoplay_ = false;
    bool controls_ = false;
    std::string preload_ = "auto";
    std::string crossOrigin_;
    bool seeking_ = false;
    TimeRanges buffered_;
    TimeRanges played_;
    std::vector<TextTrack> textTracks_;

    std::unordered_map<MediaEvent, std::vector<MediaEventCallback>> eventListeners_;
    mutable std::mutex mutex_;

    void fireEvent(MediaEvent event);
};

// ==================================================================
// Audio context (Web Audio API — simplified)
// ==================================================================

class AudioNode {
public:
    virtual ~AudioNode() = default;
    virtual void process(AudioFrame& frame) = 0;

    void connect(AudioNode* destination) { destination_ = destination; }
    AudioNode* destination() const { return destination_; }

private:
    AudioNode* destination_ = nullptr;
};

class GainNode : public AudioNode {
public:
    void setGain(float g) { gain_ = g; }
    float gain() const { return gain_; }
    void process(AudioFrame& frame) override;

private:
    float gain_ = 1.0f;
};

class BiquadFilterNode : public AudioNode {
public:
    enum class FilterType { LowPass, HighPass, BandPass, LowShelf, HighShelf, Peaking, Notch, AllPass };

    void setType(FilterType t) { type_ = t; }
    void setFrequency(float f) { frequency_ = f; }
    void setQ(float q) { q_ = q; }
    void setGain(float g) { gain_ = g; }
    void process(AudioFrame& frame) override;

private:
    FilterType type_ = FilterType::LowPass;
    float frequency_ = 350;
    float q_ = 1;
    float gain_ = 0;
};

class OscillatorNode : public AudioNode {
public:
    enum class WaveType { Sine, Square, Sawtooth, Triangle, Custom };

    void setType(WaveType t) { type_ = t; }
    void setFrequency(float f) { frequency_ = f; }
    void setDetune(float d) { detune_ = d; }
    void start(double when = 0);
    void stop(double when = 0);
    void process(AudioFrame& frame) override;

private:
    WaveType type_ = WaveType::Sine;
    float frequency_ = 440;
    float detune_ = 0;
    double phase_ = 0;
    bool running_ = false;
};

class AudioContext {
public:
    AudioContext();
    ~AudioContext();

    float sampleRate() const { return sampleRate_; }
    double currentTime() const { return currentTime_; }
    std::string state() const { return state_; }

    void resume();
    void suspend();
    void close();

    std::unique_ptr<GainNode> createGain();
    std::unique_ptr<BiquadFilterNode> createBiquadFilter();
    std::unique_ptr<OscillatorNode> createOscillator();

    AudioNode* destination() { return &destinationNode_; }

    // Decode audio data
    void decodeAudioData(const std::vector<uint8_t>& data,
                          std::function<void(AudioFrame)> onSuccess,
                          std::function<void(std::string)> onError);

private:
    float sampleRate_ = 44100;
    double currentTime_ = 0;
    std::string state_ = "suspended";

    class DestinationNode : public AudioNode {
    public:
        void process(AudioFrame& frame) override;
    };
    DestinationNode destinationNode_;
};

} // namespace Media
} // namespace NXRender
