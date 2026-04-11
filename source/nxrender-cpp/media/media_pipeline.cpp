// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "media_pipeline.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace NXRender {
namespace Media {

// ==================================================================
// VideoFrameQueue
// ==================================================================

VideoFrameQueue::VideoFrameQueue(size_t capacity) : capacity_(capacity) {}
VideoFrameQueue::~VideoFrameQueue() {}

bool VideoFrameQueue::push(VideoFrame&& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) return false;
    queue_.push_back(std::move(frame));
    return true;
}

bool VideoFrameQueue::pop(VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    frame = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

VideoFrame* VideoFrameQueue::peek() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return nullptr;
    return &queue_.front();
}

void VideoFrameQueue::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
}

size_t VideoFrameQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool VideoFrameQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

bool VideoFrameQueue::full() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() >= capacity_;
}

bool VideoFrameQueue::waitForFrame(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    while (empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

bool VideoFrameQueue::getFrameAt(double timestamp, VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;

    // Find closest frame
    size_t bestIdx = 0;
    double bestDist = std::abs(queue_[0].timestamp - timestamp);
    for (size_t i = 1; i < queue_.size(); i++) {
        double dist = std::abs(queue_[i].timestamp - timestamp);
        if (dist < bestDist) { bestDist = dist; bestIdx = i; }
    }

    frame = queue_[bestIdx];
    return true;
}

int VideoFrameQueue::dropBefore(double timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    int dropped = 0;
    while (queue_.size() > 1 && queue_.front().timestamp < timestamp) {
        queue_.pop_front();
        dropped++;
    }
    return dropped;
}

// ==================================================================
// AudioFrameQueue
// ==================================================================

AudioFrameQueue::AudioFrameQueue(size_t capacity) : capacity_(capacity) {}
AudioFrameQueue::~AudioFrameQueue() {}

bool AudioFrameQueue::push(AudioFrame&& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) return false;
    queue_.push_back(std::move(frame));
    return true;
}

bool AudioFrameQueue::pop(AudioFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    frame = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

void AudioFrameQueue::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
}

size_t AudioFrameQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool AudioFrameQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

void AudioFrameQueue::mixInto(float* output, int sampleCount, int channels, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    int samplesNeeded = sampleCount * channels;
    int samplesWritten = 0;

    while (!queue_.empty() && samplesWritten < samplesNeeded) {
        auto& frame = queue_.front();
        int available = static_cast<int>(frame.samples.size()) - 0;
        int toWrite = std::min(available, samplesNeeded - samplesWritten);

        for (int i = 0; i < toWrite; i++) {
            output[samplesWritten + i] += frame.samples[i] * volume;
        }
        samplesWritten += toWrite;

        if (toWrite >= available) {
            queue_.pop_front();
        } else {
            // Partial consume — shift remaining samples
            frame.samples.erase(frame.samples.begin(), frame.samples.begin() + toWrite);
        }
    }
}

double AudioFrameQueue::bufferedDuration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return 0;
    double first = queue_.front().timestamp;
    double last = queue_.back().timestamp;
    if (queue_.back().sampleCount > 0 && queue_.back().channels > 0) {
        last += static_cast<double>(queue_.back().sampleCount) / 44100.0;
    }
    return last - first;
}

// ==================================================================
// MediaPipeline
// ==================================================================

MediaPipeline::MediaPipeline() {}
MediaPipeline::~MediaPipeline() { close(); }

void MediaPipeline::setState(State s) {
    if (state_ == s) return;
    state_ = s;
    if (onStateChange_) onStateChange_(s);
}

bool MediaPipeline::open(const std::string& url) {
    if (!demuxer_) return false;
    setState(State::Opening);

    if (!demuxer_->openUrl(url)) {
        setState(State::Error);
        return false;
    }

    // Configure decoders from stream info
    for (int i = 0; i < demuxer_->streamCount(); i++) {
        auto info = demuxer_->streamInfo(i);
        if (info.type == DemuxedPacket::StreamType::Video && videoDecoder_) {
            VideoDecoder::Config cfg;
            cfg.codec = info.codec;
            cfg.width = info.width;
            cfg.height = info.height;
            if (videoDecoder_->configure(cfg)) hasVideo_ = true;
        }
        if (info.type == DemuxedPacket::StreamType::Audio && audioDecoder_) {
            AudioDecoder::Config cfg;
            cfg.codec = info.codec;
            cfg.sampleRate = info.sampleRate;
            cfg.channels = info.channels;
            if (audioDecoder_->configure(cfg)) hasAudio_ = true;
        }
    }

    setState(State::Ready);
    return true;
}

bool MediaPipeline::openData(const uint8_t* data, size_t size) {
    if (!demuxer_) return false;
    setState(State::Opening);

    if (!demuxer_->open(data, size)) {
        setState(State::Error);
        return false;
    }

    setState(State::Ready);
    return true;
}

void MediaPipeline::close() {
    if (demuxer_) demuxer_->close();
    if (videoDecoder_) videoDecoder_->reset();
    if (audioDecoder_) audioDecoder_->reset();
    videoQueue_.flush();
    audioQueue_.flush();
    currentTime_ = 0;
    hasVideo_ = false;
    hasAudio_ = false;
    setState(State::Idle);
}

void MediaPipeline::play() {
    if (state_ == State::Ready || state_ == State::Paused) {
        setState(State::Playing);
    }
}

void MediaPipeline::pause() {
    if (state_ == State::Playing) {
        setState(State::Paused);
    }
}

void MediaPipeline::seek(double timeSeconds) {
    if (!demuxer_) return;
    setState(State::Seeking);

    videoQueue_.flush();
    audioQueue_.flush();
    if (videoDecoder_) videoDecoder_->flush();
    if (audioDecoder_) audioDecoder_->flush();

    demuxer_->seek(timeSeconds);
    currentTime_ = timeSeconds;

    // Re-fill buffers
    pumpDemuxer();
    setState(State::Playing);
}

void MediaPipeline::setPlaybackRate(double rate) {
    playbackRate_ = std::clamp(rate, 0.25, 4.0);
}

double MediaPipeline::duration() const {
    return demuxer_ ? demuxer_->duration() : 0;
}

VideoFrame* MediaPipeline::currentVideoFrame() {
    return videoQueue_.peek();
}

void MediaPipeline::advanceVideoFrame() {
    VideoFrame frame;
    videoQueue_.pop(frame);
}

void MediaPipeline::setVideoDecoder(std::unique_ptr<VideoDecoder> decoder) {
    videoDecoder_ = std::move(decoder);
}

void MediaPipeline::setAudioDecoder(std::unique_ptr<AudioDecoder> decoder) {
    audioDecoder_ = std::move(decoder);
}

void MediaPipeline::setDemuxer(std::unique_ptr<Demuxer> demuxer) {
    demuxer_ = std::move(demuxer);
}

void MediaPipeline::tick(double deltaMs) {
    if (state_ != State::Playing) return;

    currentTime_ += (deltaMs / 1000.0) * playbackRate_;

    // Check end of stream
    if (demuxer_ && demuxer_->isEOF() && videoQueue_.empty() && audioQueue_.empty()) {
        setState(State::Ended);
        return;
    }

    // Keep buffers filled
    pumpDemuxer();

    // Drop late video frames
    if (hasVideo_) {
        int dropped = videoQueue_.dropBefore(currentTime_ - 0.033); // 1 frame tolerance
        stats_.videoFramesDropped += dropped;
    }

    syncAudioVideo();
}

void MediaPipeline::pumpDemuxer() {
    if (!demuxer_ || demuxer_->isEOF()) return;

    // Fill queues up to capacity
    int maxPackets = 4;
    for (int i = 0; i < maxPackets; i++) {
        if (videoQueue_.full() && audioQueue_.size() >= 8) break;

        DemuxedPacket packet;
        if (!demuxer_->readPacket(packet)) break;

        if (packet.type == DemuxedPacket::StreamType::Video && videoDecoder_) {
            if (videoDecoder_->decode(packet.data.data(), packet.data.size(),
                                      packet.pts, packet.keyframe)) {
                VideoFrame* frame;
                while ((frame = videoDecoder_->dequeueFrame()) != nullptr) {
                    videoQueue_.push(std::move(*frame));
                    stats_.videoFramesDecoded++;
                }
            }
        }

        if (packet.type == DemuxedPacket::StreamType::Audio && audioDecoder_) {
            if (audioDecoder_->decode(packet.data.data(), packet.data.size(), packet.pts)) {
                AudioFrame* frame;
                while ((frame = audioDecoder_->dequeueFrame()) != nullptr) {
                    audioQueue_.push(std::move(*frame));
                    stats_.audioFramesDecoded++;
                }
            }
        }
    }
}

void MediaPipeline::syncAudioVideo() {
    // A/V sync: adjust video presentation based on audio clock
    if (!hasAudio_ || !hasVideo_) return;

    auto* vFrame = videoQueue_.peek();
    if (!vFrame) return;

    double drift = vFrame->timestamp - currentTime_;
    if (drift > 0.05) {
        // Video ahead — wait
    } else if (drift < -0.1) {
        // Video behind — drop frame
        advanceVideoFrame();
        stats_.videoFramesDropped++;
    }

    stats_.bufferHealth = audioQueue_.bufferedDuration();
}

// ==================================================================
// SourceBuffer
// ==================================================================

SourceBuffer::SourceBuffer() {}
SourceBuffer::~SourceBuffer() {}

void SourceBuffer::appendBuffer(const uint8_t* data, size_t size) {
    if (updating_) return;
    updating_ = true;
    buffer_.insert(buffer_.end(), data, data + size);
    updating_ = false;
    if (onUpdateEnd_) onUpdateEnd_();
}

void SourceBuffer::remove(double start, double end) {
    (void)start; (void)end;
    // Would remove frames in time range from internal storage
}

void SourceBuffer::abort() {
    updating_ = false;
}

// ==================================================================
// MediaSourceAPI
// ==================================================================

MediaSourceAPI::MediaSourceAPI() {}
MediaSourceAPI::~MediaSourceAPI() {}

SourceBuffer* MediaSourceAPI::addSourceBuffer(const std::string& mimeType) {
    (void)mimeType;
    auto sb = std::make_unique<SourceBuffer>();
    SourceBuffer* ptr = sb.get();
    sourceBuffers_.push_back(std::move(sb));
    readyState_ = ReadyState::Open;
    return ptr;
}

void MediaSourceAPI::removeSourceBuffer(SourceBuffer* sb) {
    sourceBuffers_.erase(
        std::remove_if(sourceBuffers_.begin(), sourceBuffers_.end(),
                        [sb](const std::unique_ptr<SourceBuffer>& p) { return p.get() == sb; }),
        sourceBuffers_.end());
}

void MediaSourceAPI::endOfStream() {
    readyState_ = ReadyState::Ended;
}

void MediaSourceAPI::endOfStream(const std::string& /*error*/) {
    readyState_ = ReadyState::Ended;
}

std::string MediaSourceAPI::createObjectURL(MediaSourceAPI* /*ms*/) {
    static int counter = 0;
    return "blob:nxrender://media-source/" + std::to_string(counter++);
}

bool MediaSourceAPI::isTypeSupported(const std::string& mimeType) {
    // Supported types for Zepra
    return mimeType.find("video/mp4") != std::string::npos ||
           mimeType.find("video/webm") != std::string::npos ||
           mimeType.find("audio/mp4") != std::string::npos ||
           mimeType.find("audio/webm") != std::string::npos;
}

// ==================================================================
// MediaCapabilities
// ==================================================================

MediaCapabilities::DecodingInfo MediaCapabilities::decodingInfo(
    const std::string& mimeType, int width, int height, float framerate, int bitrate) {
    DecodingInfo info;
    (void)bitrate;

    info.supported = MediaSourceAPI::isTypeSupported(mimeType) ||
                     canPlayType(mimeType);

    // Conservative estimates for software decode
    info.smooth = (width <= 1920 && height <= 1080 && framerate <= 60);
    info.powerEfficient = (width <= 1280 && height <= 720 && framerate <= 30);

    return info;
}

std::vector<CodecCapability> MediaCapabilities::supportedCodecs() {
    return {
        {CodecType::H264, "video/mp4; codecs=\"avc1.42E01E\"", 3840, 2160, 0, false, true},
        {CodecType::VP9,  "video/webm; codecs=\"vp9\"", 3840, 2160, 0, false, true},
        {CodecType::AV1,  "video/mp4; codecs=\"av01.0.08M.08\"", 1920, 1080, 0, false, true},
        {CodecType::AAC,  "audio/mp4; codecs=\"mp4a.40.2\"", 0, 0, 48000, false, true},
        {CodecType::Opus, "audio/webm; codecs=\"opus\"", 0, 0, 48000, false, true},
        {CodecType::MP3,  "audio/mpeg", 0, 0, 48000, false, true},
    };
}

bool MediaCapabilities::canPlayType(const std::string& mimeType) {
    auto codecs = supportedCodecs();
    for (const auto& c : codecs) {
        if (mimeType.find(c.mimeType.substr(0, c.mimeType.find(';'))) != std::string::npos) {
            return c.supported;
        }
    }
    return false;
}

// ==================================================================
// PictureInPicture
// ==================================================================

bool PictureInPicture::isSupported() const { return true; }

void PictureInPicture::enter(MediaElement* element) {
    element_ = element;
    active_ = true;
    if (onEnter_) onEnter_();
}

void PictureInPicture::exit() {
    active_ = false;
    element_ = nullptr;
    if (onLeave_) onLeave_();
}

void PictureInPicture::resize(int w, int h) {
    width_ = w;
    height_ = h;
    if (onResize_) onResize_();
}

} // namespace Media
} // namespace NXRender
