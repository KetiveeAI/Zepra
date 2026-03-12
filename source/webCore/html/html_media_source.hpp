/**
 * @file html_media_source.hpp
 * @brief Media Source Extensions (MSE) interfaces
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace Zepra::WebCore {

/**
 * @brief Media source ready state
 */
enum class MediaSourceReadyState {
    Closed,
    Open,
    Ended
};

/**
 * @brief Append mode for source buffer
 */
enum class AppendMode {
    Segments,
    Sequence
};

/**
 * @brief Time range
 */
struct TimeRange {
    double start;
    double end;
};

/**
 * @brief Time ranges collection
 */
class TimeRanges {
public:
    size_t length() const { return ranges_.size(); }
    double start(size_t index) const { return ranges_[index].start; }
    double end(size_t index) const { return ranges_[index].end; }
    
    void add(double start, double end) {
        ranges_.push_back({start, end});
    }
    
private:
    std::vector<TimeRange> ranges_;
};

/**
 * @brief Source buffer for MSE
 */
class SourceBuffer {
public:
    virtual ~SourceBuffer() = default;
    
    // State
    virtual AppendMode mode() const = 0;
    virtual void setMode(AppendMode mode) = 0;
    
    virtual bool updating() const = 0;
    
    virtual const TimeRanges& buffered() const = 0;
    
    virtual double timestampOffset() const = 0;
    virtual void setTimestampOffset(double offset) = 0;
    
    virtual double appendWindowStart() const = 0;
    virtual void setAppendWindowStart(double start) = 0;
    
    virtual double appendWindowEnd() const = 0;
    virtual void setAppendWindowEnd(double end) = 0;
    
    // Operations
    virtual void appendBuffer(const std::vector<uint8_t>& data) = 0;
    virtual void abort() = 0;
    virtual void remove(double start, double end) = 0;
    virtual void changeType(const std::string& type) = 0;
    
    // Events
    std::function<void()> onUpdateStart;
    std::function<void()> onUpdate;
    std::function<void()> onUpdateEnd;
    std::function<void()> onError;
    std::function<void()> onAbort;
};

/**
 * @brief Source buffer list
 */
class SourceBufferList {
public:
    size_t length() const { return buffers_.size(); }
    SourceBuffer* operator[](size_t index) { return buffers_[index].get(); }
    
    void add(std::unique_ptr<SourceBuffer> buffer) {
        buffers_.push_back(std::move(buffer));
    }
    
    void remove(SourceBuffer* buffer);
    
    std::function<void()> onAddSourceBuffer;
    std::function<void()> onRemoveSourceBuffer;
    
private:
    std::vector<std::unique_ptr<SourceBuffer>> buffers_;
};

/**
 * @brief Media source
 */
class MediaSource {
public:
    MediaSource();
    virtual ~MediaSource();
    
    // State
    MediaSourceReadyState readyState() const { return readyState_; }
    
    double duration() const { return duration_; }
    void setDuration(double d) { duration_ = d; }
    
    // Source buffers
    SourceBufferList& sourceBuffers() { return sourceBuffers_; }
    SourceBufferList& activeSourceBuffers() { return activeSourceBuffers_; }
    
    // Operations
    SourceBuffer* addSourceBuffer(const std::string& type);
    void removeSourceBuffer(SourceBuffer* buffer);
    void endOfStream(const std::string& error = "");
    void setLiveSeekableRange(double start, double end);
    void clearLiveSeekableRange();
    
    // Static methods
    static std::string createObjectURL(MediaSource* source);
    static bool isTypeSupported(const std::string& type);
    
    // Events
    std::function<void()> onSourceOpen;
    std::function<void()> onSourceEnded;
    std::function<void()> onSourceClose;
    
private:
    MediaSourceReadyState readyState_ = MediaSourceReadyState::Closed;
    double duration_ = 0;
    SourceBufferList sourceBuffers_;
    SourceBufferList activeSourceBuffers_;
};

} // namespace Zepra::WebCore
