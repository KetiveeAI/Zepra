// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "media_element.h"
#include <deque>
#include <atomic>
#include <thread>

namespace NXRender {
namespace Media {

// ==================================================================
// Codec interface — platform-specific decoders implement this
// ==================================================================

enum class CodecType : uint8_t {
    H264, H265, VP8, VP9, AV1,
    AAC, Opus, Vorbis, MP3, FLAC, PCM,
};

struct CodecCapability {
    CodecType codec;
    std::string mimeType;
    int maxWidth = 0, maxHeight = 0;
    int maxSampleRate = 0;
    bool hardwareAccelerated = false;
    bool supported = false;
};

class VideoDecoder {
public:
    virtual ~VideoDecoder() = default;

    struct Config {
        CodecType codec = CodecType::H264;
        int width = 0, height = 0;
        std::vector<uint8_t> extraData; // SPS/PPS for H.264, etc.
        bool lowLatency = false;
        bool preferHardware = true;
    };

    virtual bool configure(const Config& config) = 0;
    virtual bool decode(const uint8_t* data, size_t size, double pts, bool keyframe) = 0;
    virtual bool flush() = 0;
    virtual VideoFrame* dequeueFrame() = 0;
    virtual void reset() = 0;
    virtual bool isConfigured() const = 0;
    virtual std::string name() const = 0;
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    struct Config {
        CodecType codec = CodecType::AAC;
        int sampleRate = 44100;
        int channels = 2;
        std::vector<uint8_t> extraData;
    };

    virtual bool configure(const Config& config) = 0;
    virtual bool decode(const uint8_t* data, size_t size, double pts) = 0;
    virtual bool flush() = 0;
    virtual AudioFrame* dequeueFrame() = 0;
    virtual void reset() = 0;
    virtual bool isConfigured() const = 0;
    virtual std::string name() const = 0;
};

// ==================================================================
// Demuxer — container format parser interface
// ==================================================================

enum class ContainerFormat : uint8_t {
    Unknown, MP4, WebM, MKV, OGG, WAV, MP3, FLAC, HLS, DASH
};

struct DemuxedPacket {
    enum class StreamType { Audio, Video, Subtitle } type = StreamType::Video;
    std::vector<uint8_t> data;
    double pts = 0;
    double dts = 0;
    double duration = 0;
    bool keyframe = false;
    int streamIndex = 0;
};

struct StreamInfo {
    DemuxedPacket::StreamType type;
    int index = 0;
    CodecType codec;
    double duration = 0;
    std::string language;

    // Video-specific
    int width = 0, height = 0;
    float frameRate = 0;
    float aspectRatio = 0;

    // Audio-specific
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 0;
};

class Demuxer {
public:
    virtual ~Demuxer() = default;

    virtual bool open(const uint8_t* data, size_t size) = 0;
    virtual bool openUrl(const std::string& url) = 0;
    virtual ContainerFormat format() const = 0;

    virtual int streamCount() const = 0;
    virtual StreamInfo streamInfo(int index) const = 0;
    virtual int bestVideoStream() const = 0;
    virtual int bestAudioStream() const = 0;

    virtual bool readPacket(DemuxedPacket& packet) = 0;
    virtual bool seek(double timeSeconds) = 0;
    virtual double duration() const = 0;

    virtual bool isEOF() const = 0;
    virtual void close() = 0;
};

// ==================================================================
// Video frame queue — thread-safe ring buffer
// ==================================================================

class VideoFrameQueue {
public:
    VideoFrameQueue(size_t capacity = 8);
    ~VideoFrameQueue();

    bool push(VideoFrame&& frame);
    bool pop(VideoFrame& frame);
    VideoFrame* peek();
    void flush();
    size_t size() const;
    bool empty() const;
    bool full() const;
    size_t capacity() const { return capacity_; }

    // Wait for frame with timeout
    bool waitForFrame(int timeoutMs);

    // Get frame closest to target timestamp
    bool getFrameAt(double timestamp, VideoFrame& frame);

    // Drop frames before timestamp
    int dropBefore(double timestamp);

private:
    std::deque<VideoFrame> queue_;
    size_t capacity_;
    mutable std::mutex mutex_;
};

class AudioFrameQueue {
public:
    AudioFrameQueue(size_t capacity = 16);
    ~AudioFrameQueue();

    bool push(AudioFrame&& frame);
    bool pop(AudioFrame& frame);
    void flush();
    size_t size() const;
    bool empty() const;

    // Mix multiple frames into output buffer
    void mixInto(float* output, int sampleCount, int channels, float volume);

    // Get buffered duration in seconds
    double bufferedDuration() const;

private:
    std::deque<AudioFrame> queue_;
    size_t capacity_;
    mutable std::mutex mutex_;
};

// ==================================================================
// Media pipeline — connects demuxer → decoder → frame queues
// ==================================================================

class MediaPipeline {
public:
    MediaPipeline();
    ~MediaPipeline();

    enum class State {
        Idle, Opening, Buffering, Ready, Playing, Paused, Seeking, Ended, Error
    };

    bool open(const std::string& url);
    bool openData(const uint8_t* data, size_t size);
    void close();

    void play();
    void pause();
    void seek(double timeSeconds);
    void setPlaybackRate(double rate);

    State state() const { return state_; }
    double currentTime() const { return currentTime_; }
    double duration() const;
    bool hasVideo() const { return hasVideo_; }
    bool hasAudio() const { return hasAudio_; }

    // Frame access
    VideoFrame* currentVideoFrame();
    void advanceVideoFrame();

    // Volume
    void setVolume(float vol) { volume_ = vol; }
    float volume() const { return volume_; }

    // Decoder registration
    void setVideoDecoder(std::unique_ptr<VideoDecoder> decoder);
    void setAudioDecoder(std::unique_ptr<AudioDecoder> decoder);
    void setDemuxer(std::unique_ptr<Demuxer> demuxer);

    // Tick — call each frame to pump the pipeline
    void tick(double deltaMs);

    // Callbacks
    using StateCallback = std::function<void(State)>;
    void onStateChange(StateCallback cb) { onStateChange_ = cb; }

    // Stats
    struct Stats {
        int videoFramesDecoded = 0;
        int videoFramesDropped = 0;
        int audioFramesDecoded = 0;
        double decodeFps = 0;
        double bufferHealth = 0; // seconds of buffered content
    };
    Stats stats() const { return stats_; }

private:
    State state_ = State::Idle;
    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<VideoDecoder> videoDecoder_;
    std::unique_ptr<AudioDecoder> audioDecoder_;
    VideoFrameQueue videoQueue_;
    AudioFrameQueue audioQueue_;

    double currentTime_ = 0;
    double playbackRate_ = 1.0;
    float volume_ = 1.0f;
    bool hasVideo_ = false;
    bool hasAudio_ = false;

    StateCallback onStateChange_;
    Stats stats_;

    void setState(State s);
    void pumpDemuxer();
    void syncAudioVideo();
};

// ==================================================================
// Media Source Extensions (MSE)
// ==================================================================

class SourceBuffer {
public:
    SourceBuffer();
    ~SourceBuffer();

    void appendBuffer(const uint8_t* data, size_t size);
    void remove(double start, double end);
    void abort();

    bool updating() const { return updating_; }
    TimeRanges buffered() const { return buffered_; }
    double timestampOffset() const { return timestampOffset_; }
    void setTimestampOffset(double offset) { timestampOffset_ = offset; }

    std::string mode() const { return mode_; }
    void setMode(const std::string& m) { mode_ = m; }

    using UpdateCallback = std::function<void()>;
    void onUpdateEnd(UpdateCallback cb) { onUpdateEnd_ = cb; }

private:
    std::vector<uint8_t> buffer_;
    bool updating_ = false;
    TimeRanges buffered_;
    double timestampOffset_ = 0;
    std::string mode_ = "segments";
    UpdateCallback onUpdateEnd_;
};

class MediaSourceAPI {
public:
    MediaSourceAPI();
    ~MediaSourceAPI();

    enum class ReadyState { Closed, Open, Ended };

    ReadyState readyState() const { return readyState_; }
    double duration() const { return duration_; }
    void setDuration(double d) { duration_ = d; }

    SourceBuffer* addSourceBuffer(const std::string& mimeType);
    void removeSourceBuffer(SourceBuffer* sb);
    const std::vector<std::unique_ptr<SourceBuffer>>& sourceBuffers() const { return sourceBuffers_; }

    void endOfStream();
    void endOfStream(const std::string& error);

    std::string objectURL() const { return objectURL_; }
    static std::string createObjectURL(MediaSourceAPI* ms);
    static bool isTypeSupported(const std::string& mimeType);

private:
    ReadyState readyState_ = ReadyState::Closed;
    double duration_ = 0;
    std::vector<std::unique_ptr<SourceBuffer>> sourceBuffers_;
    std::string objectURL_;
};

// ==================================================================
// Media capabilities API
// ==================================================================

struct MediaCapabilities {
    struct DecodingInfo {
        bool supported = false;
        bool smooth = false;
        bool powerEfficient = false;
    };

    static DecodingInfo decodingInfo(const std::string& mimeType,
                                      int width = 0, int height = 0,
                                      float framerate = 0, int bitrate = 0);
    static std::vector<CodecCapability> supportedCodecs();
    static bool canPlayType(const std::string& mimeType);
};

// ==================================================================
// Picture-in-Picture
// ==================================================================

class PictureInPicture {
public:
    bool isSupported() const;
    bool isActive() const { return active_; }

    void enter(MediaElement* element);
    void exit();

    int width() const { return width_; }
    int height() const { return height_; }
    void resize(int w, int h);

    using EventCallback = std::function<void()>;
    void onEnter(EventCallback cb) { onEnter_ = cb; }
    void onLeave(EventCallback cb) { onLeave_ = cb; }
    void onResize(EventCallback cb) { onResize_ = cb; }

private:
    bool active_ = false;
    MediaElement* element_ = nullptr;
    int width_ = 320, height_ = 180;
    EventCallback onEnter_, onLeave_, onResize_;
};

} // namespace Media
} // namespace NXRender
