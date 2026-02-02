/**
 * @file BlobAPI.h
 * @brief Blob and File API Implementation
 * 
 * File API:
 * - Blob: Binary data container
 * - File: File metadata + blob
 * - FileReader: Read file contents
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include <chrono>

namespace Zepra::API {

// =============================================================================
// Blob
// =============================================================================

/**
 * @brief Immutable raw binary data
 */
class Blob {
public:
    Blob() = default;
    
    // From raw bytes
    explicit Blob(std::vector<uint8_t> data, const std::string& type = "")
        : data_(std::make_shared<std::vector<uint8_t>>(std::move(data)))
        , type_(type) {}
    
    // From string
    explicit Blob(const std::string& text, const std::string& type = "text/plain")
        : data_(std::make_shared<std::vector<uint8_t>>(text.begin(), text.end()))
        , type_(type) {}
    
    // Properties
    size_t size() const { return data_ ? data_->size() : 0; }
    const std::string& type() const { return type_; }
    
    // Slice
    Blob slice(size_t start = 0, size_t end = SIZE_MAX,
               const std::string& contentType = "") const {
        if (!data_) return Blob();
        
        size_t actualEnd = std::min(end, data_->size());
        size_t actualStart = std::min(start, actualEnd);
        
        std::vector<uint8_t> sliced(
            data_->begin() + actualStart,
            data_->begin() + actualEnd);
        
        return Blob(std::move(sliced), 
                    contentType.empty() ? type_ : contentType);
    }
    
    // Get raw data
    const std::vector<uint8_t>& data() const {
        static std::vector<uint8_t> empty;
        return data_ ? *data_ : empty;
    }
    
    // Convert to string
    std::string text() const {
        if (!data_) return "";
        return std::string(data_->begin(), data_->end());
    }
    
    // Convert to ArrayBuffer equivalent
    std::vector<uint8_t> arrayBuffer() const {
        return data_ ? *data_ : std::vector<uint8_t>{};
    }
    
    // Stream
    // ReadableStream stream() const;
    
protected:
    std::shared_ptr<std::vector<uint8_t>> data_;
    std::string type_;
};

// =============================================================================
// File
// =============================================================================

/**
 * @brief File with metadata
 */
class File : public Blob {
public:
    File() = default;
    
    File(std::vector<uint8_t> data, const std::string& name,
         const std::string& type = "",
         int64_t lastModified = 0)
        : Blob(std::move(data), type)
        , name_(name)
        , lastModified_(lastModified == 0 ? currentTime() : lastModified) {}
    
    // Properties
    const std::string& name() const { return name_; }
    int64_t lastModified() const { return lastModified_; }
    
    // Webkit-style path (not standard)
    const std::string& webkitRelativePath() const { return relativePath_; }
    void setWebkitRelativePath(const std::string& path) { relativePath_ = path; }
    
private:
    static int64_t currentTime() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }
    
    std::string name_;
    int64_t lastModified_ = 0;
    std::string relativePath_;
};

// =============================================================================
// FileReader
// =============================================================================

enum class FileReaderState : uint8_t {
    Empty = 0,
    Loading = 1,
    Done = 2
};

/**
 * @brief Read file contents asynchronously
 */
class FileReader {
public:
    using ProgressCallback = std::function<void(size_t loaded, size_t total)>;
    using LoadCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    FileReader() = default;
    
    // State
    FileReaderState readyState() const { return state_; }
    const std::string& error() const { return error_; }
    
    // Result
    const std::vector<uint8_t>& result() const { return result_; }
    std::string resultAsText() const {
        return std::string(result_.begin(), result_.end());
    }
    std::string resultAsDataURL() const {
        // Would base64 encode
        return "data:" + type_ + ";base64,...";
    }
    
    // Read methods
    void readAsArrayBuffer(const Blob& blob) {
        startRead(blob);
        result_ = blob.data();
        finishRead();
    }
    
    void readAsText(const Blob& blob, const std::string& encoding = "UTF-8") {
        startRead(blob);
        result_ = blob.data();
        finishRead();
    }
    
    void readAsDataURL(const Blob& blob) {
        startRead(blob);
        type_ = blob.type();
        result_ = blob.data();
        finishRead();
    }
    
    void readAsBinaryString(const Blob& blob) {
        startRead(blob);
        result_ = blob.data();
        finishRead();
    }
    
    // Abort
    void abort() {
        if (state_ == FileReaderState::Loading) {
            state_ = FileReaderState::Done;
            error_ = "Aborted";
            if (onabort) onabort();
        }
    }
    
    // Event handlers
    LoadCallback onloadstart;
    ProgressCallback onprogress;
    LoadCallback onload;
    LoadCallback onloadend;
    ErrorCallback onerror;
    LoadCallback onabort;
    
private:
    void startRead(const Blob& blob) {
        state_ = FileReaderState::Loading;
        error_.clear();
        result_.clear();
        if (onloadstart) onloadstart();
    }
    
    void finishRead() {
        state_ = FileReaderState::Done;
        if (onload) onload();
        if (onloadend) onloadend();
    }
    
    FileReaderState state_ = FileReaderState::Empty;
    std::string error_;
    std::vector<uint8_t> result_;
    std::string type_;
};

// =============================================================================
// URL.createObjectURL / revokeObjectURL
// =============================================================================

class BlobURLStore {
public:
    static BlobURLStore& instance() {
        static BlobURLStore store;
        return store;
    }
    
    std::string createObjectURL(const Blob& blob) {
        std::string id = "blob:" + std::to_string(nextId_++);
        blobs_[id] = std::make_shared<Blob>(blob);
        return id;
    }
    
    void revokeObjectURL(const std::string& url) {
        blobs_.erase(url);
    }
    
    std::shared_ptr<Blob> getBlob(const std::string& url) const {
        auto it = blobs_.find(url);
        return it != blobs_.end() ? it->second : nullptr;
    }
    
private:
    BlobURLStore() = default;
    size_t nextId_ = 1;
    std::unordered_map<std::string, std::shared_ptr<Blob>> blobs_;
};

} // namespace Zepra::API
