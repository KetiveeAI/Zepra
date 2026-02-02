/**
 * @file StreamsAPI.h
 * @brief Streams API Implementation
 * 
 * WHATWG Streams Standard:
 * - ReadableStream: Async readable data
 * - WritableStream: Async writable data
 * - TransformStream: Transform pipe
 */

#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <cstdint>
#include <optional>

namespace Zepra::API {

// Forward declarations
class ReadableStreamDefaultReader;
class WritableStreamDefaultWriter;

// =============================================================================
// Chunk Types
// =============================================================================

using Chunk = std::vector<uint8_t>;

// =============================================================================
// Underlying Source (for ReadableStream)
// =============================================================================

class ReadableStreamController;

/**
 * @brief Source of data for ReadableStream
 */
struct UnderlyingSource {
    std::function<void(ReadableStreamController*)> start;
    std::function<void(ReadableStreamController*)> pull;
    std::function<void(const std::string&)> cancel;
    std::string type;  // "bytes" for byte streams
};

/**
 * @brief Controller for ReadableStream
 */
class ReadableStreamController {
public:
    void enqueue(Chunk chunk) {
        queue_.push(std::move(chunk));
    }
    
    void close() {
        closed_ = true;
    }
    
    void error(const std::string& reason) {
        errored_ = true;
        errorReason_ = reason;
    }
    
    size_t desiredSize() const {
        return highWaterMark_ - queueSize();
    }
    
    bool isClosed() const { return closed_; }
    bool isErrored() const { return errored_; }
    
    std::optional<Chunk> dequeue() {
        if (queue_.empty()) return std::nullopt;
        Chunk c = std::move(queue_.front());
        queue_.pop();
        return c;
    }
    
    bool hasChunks() const { return !queue_.empty(); }
    
private:
    size_t queueSize() const {
        size_t size = 0;
        // Would calculate total bytes
        return size;
    }
    
    std::queue<Chunk> queue_;
    size_t highWaterMark_ = 1;
    bool closed_ = false;
    bool errored_ = false;
    std::string errorReason_;
};

// =============================================================================
// ReadableStream
// =============================================================================

enum class ReadableStreamState {
    Readable,
    Closed,
    Errored
};

/**
 * @brief Readable stream of data
 */
class ReadableStream {
public:
    ReadableStream() = default;
    
    explicit ReadableStream(UnderlyingSource source, size_t highWaterMark = 1) {
        controller_ = std::make_unique<ReadableStreamController>();
        if (source.start) {
            source.start(controller_.get());
        }
        pullFn_ = source.pull;
        cancelFn_ = source.cancel;
    }
    
    // Properties
    bool locked() const { return locked_; }
    
    // Get reader
    std::unique_ptr<ReadableStreamDefaultReader> getReader();
    
    // Pipe to writable
    // pipeTo(WritableStream dest);
    
    // Cancel stream
    void cancel(const std::string& reason = "") {
        if (cancelFn_) {
            cancelFn_(reason);
        }
        state_ = ReadableStreamState::Closed;
    }
    
    // Tee (split into two streams)
    // std::pair<ReadableStream, ReadableStream> tee();
    
    ReadableStreamState state() const { return state_; }
    
private:
    friend class ReadableStreamDefaultReader;
    
    std::unique_ptr<ReadableStreamController> controller_;
    std::function<void(ReadableStreamController*)> pullFn_;
    std::function<void(const std::string&)> cancelFn_;
    ReadableStreamState state_ = ReadableStreamState::Readable;
    bool locked_ = false;
};

// =============================================================================
// ReadableStreamDefaultReader
// =============================================================================

struct ReadResult {
    std::optional<Chunk> value;
    bool done;
};

/**
 * @brief Reader for ReadableStream
 */
class ReadableStreamDefaultReader {
public:
    explicit ReadableStreamDefaultReader(ReadableStream* stream)
        : stream_(stream) {
        stream_->locked_ = true;
    }
    
    ~ReadableStreamDefaultReader() {
        releaseLock();
    }
    
    // Read next chunk
    ReadResult read() {
        if (!stream_ || stream_->state_ == ReadableStreamState::Closed) {
            return {std::nullopt, true};
        }
        
        if (stream_->controller_->hasChunks()) {
            return {stream_->controller_->dequeue(), false};
        }
        
        // Pull more data
        if (stream_->pullFn_) {
            stream_->pullFn_(stream_->controller_.get());
        }
        
        if (stream_->controller_->hasChunks()) {
            return {stream_->controller_->dequeue(), false};
        }
        
        if (stream_->controller_->isClosed()) {
            return {std::nullopt, true};
        }
        
        return {std::nullopt, false};
    }
    
    // Cancel reading
    void cancel(const std::string& reason = "") {
        if (stream_) {
            stream_->cancel(reason);
        }
    }
    
    // Release lock
    void releaseLock() {
        if (stream_) {
            stream_->locked_ = false;
            stream_ = nullptr;
        }
    }
    
    bool closed() const {
        return !stream_ || stream_->state_ == ReadableStreamState::Closed;
    }
    
private:
    ReadableStream* stream_;
};

// =============================================================================
// Underlying Sink (for WritableStream)
// =============================================================================

class WritableStreamController;

/**
 * @brief Sink for WritableStream
 */
struct UnderlyingSink {
    std::function<void(WritableStreamController*)> start;
    std::function<void(Chunk, WritableStreamController*)> write;
    std::function<void()> close;
    std::function<void(const std::string&)> abort;
};

/**
 * @brief Controller for WritableStream
 */
class WritableStreamController {
public:
    void error(const std::string& reason) {
        errored_ = true;
        errorReason_ = reason;
    }
    
    bool isErrored() const { return errored_; }
    
private:
    bool errored_ = false;
    std::string errorReason_;
};

// =============================================================================
// WritableStream
// =============================================================================

enum class WritableStreamState {
    Writable,
    Closed,
    Errored
};

/**
 * @brief Writable stream of data
 */
class WritableStream {
public:
    WritableStream() = default;
    
    explicit WritableStream(UnderlyingSink sink, size_t highWaterMark = 1) {
        controller_ = std::make_unique<WritableStreamController>();
        if (sink.start) {
            sink.start(controller_.get());
        }
        writeFn_ = sink.write;
        closeFn_ = sink.close;
        abortFn_ = sink.abort;
    }
    
    // Properties
    bool locked() const { return locked_; }
    
    // Get writer
    std::unique_ptr<WritableStreamDefaultWriter> getWriter();
    
    // Abort stream
    void abort(const std::string& reason = "") {
        if (abortFn_) {
            abortFn_(reason);
        }
        state_ = WritableStreamState::Errored;
    }
    
    // Close stream
    void close() {
        if (closeFn_) {
            closeFn_();
        }
        state_ = WritableStreamState::Closed;
    }
    
    WritableStreamState state() const { return state_; }
    
private:
    friend class WritableStreamDefaultWriter;
    
    std::unique_ptr<WritableStreamController> controller_;
    std::function<void(Chunk, WritableStreamController*)> writeFn_;
    std::function<void()> closeFn_;
    std::function<void(const std::string&)> abortFn_;
    WritableStreamState state_ = WritableStreamState::Writable;
    bool locked_ = false;
};

// =============================================================================
// WritableStreamDefaultWriter
// =============================================================================

/**
 * @brief Writer for WritableStream
 */
class WritableStreamDefaultWriter {
public:
    explicit WritableStreamDefaultWriter(WritableStream* stream)
        : stream_(stream) {
        stream_->locked_ = true;
    }
    
    ~WritableStreamDefaultWriter() {
        releaseLock();
    }
    
    // Write chunk
    void write(Chunk chunk) {
        if (!stream_ || stream_->state_ != WritableStreamState::Writable) {
            throw std::runtime_error("Stream not writable");
        }
        
        if (stream_->writeFn_) {
            stream_->writeFn_(std::move(chunk), stream_->controller_.get());
        }
    }
    
    // Close stream
    void close() {
        if (stream_) {
            stream_->close();
        }
    }
    
    // Abort stream
    void abort(const std::string& reason = "") {
        if (stream_) {
            stream_->abort(reason);
        }
    }
    
    // Release lock
    void releaseLock() {
        if (stream_) {
            stream_->locked_ = false;
            stream_ = nullptr;
        }
    }
    
    bool closed() const {
        return !stream_ || stream_->state_ == WritableStreamState::Closed;
    }
    
private:
    WritableStream* stream_;
};

// =============================================================================
// TransformStream
// =============================================================================

/**
 * @brief Transform stream (pipe readable → writable with transform)
 */
class TransformStream {
public:
    using TransformFn = std::function<Chunk(const Chunk&)>;
    
    explicit TransformStream(TransformFn transform)
        : transformFn_(std::move(transform)) {}
    
    ReadableStream& readable() { return readable_; }
    WritableStream& writable() { return writable_; }
    
private:
    TransformFn transformFn_;
    ReadableStream readable_;
    WritableStream writable_;
};

// =============================================================================
// Implementation of getReader/getWriter
// =============================================================================

inline std::unique_ptr<ReadableStreamDefaultReader> ReadableStream::getReader() {
    if (locked_) throw std::runtime_error("Stream already locked");
    return std::make_unique<ReadableStreamDefaultReader>(this);
}

inline std::unique_ptr<WritableStreamDefaultWriter> WritableStream::getWriter() {
    if (locked_) throw std::runtime_error("Stream already locked");
    return std::make_unique<WritableStreamDefaultWriter>(this);
}

} // namespace Zepra::API
