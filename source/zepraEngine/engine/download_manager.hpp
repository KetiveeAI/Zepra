/**
 * @file download_manager.hpp
 * @brief Download management
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <cstdint>

namespace Zepra::Engine {

enum class DownloadState {
    Pending,
    InProgress,
    Paused,
    Completed,
    Failed,
    Cancelled
};

struct DownloadItem {
    int64_t id;
    std::string url;
    std::string filename;
    std::string savePath;
    std::string mimeType;
    
    int64_t totalBytes;
    int64_t receivedBytes;
    float progress;  // 0.0 - 1.0
    float speedBps;  // bytes per second
    
    DownloadState state;
    std::string error;
    
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
};

/**
 * @class DownloadManager
 * @brief Manages file downloads
 */
class DownloadManager {
public:
    DownloadManager();
    ~DownloadManager();
    
    // Start download
    int64_t startDownload(const std::string& url, 
                          const std::string& savePath = "");
    
    // Control
    void pause(int64_t id);
    void resume(int64_t id);
    void cancel(int64_t id);
    void remove(int64_t id);
    void retry(int64_t id);
    
    // Query
    DownloadItem* getDownload(int64_t id);
    std::vector<DownloadItem> getAllDownloads();
    std::vector<DownloadItem> getActiveDownloads();
    std::vector<DownloadItem> getCompletedDownloads();
    
    // Settings
    void setDownloadDirectory(const std::string& path);
    std::string downloadDirectory() const { return downloadDir_; }
    void setMaxConcurrentDownloads(int max) { maxConcurrent_ = max; }
    
    // Callbacks
    using ProgressCallback = std::function<void(int64_t id, float progress)>;
    using StateCallback = std::function<void(int64_t id, DownloadState state)>;
    
    void setOnProgress(ProgressCallback callback) { onProgress_ = std::move(callback); }
    void setOnStateChange(StateCallback callback) { onStateChange_ = std::move(callback); }
    
    // Clear history
    void clearCompleted();
    
private:
    std::vector<DownloadItem> downloads_;
    int64_t nextId_ = 1;
    std::string downloadDir_;
    int maxConcurrent_ = 3;
    
    ProgressCallback onProgress_;
    StateCallback onStateChange_;
};

DownloadManager& getDownloadManager();

} // namespace Zepra::Engine
