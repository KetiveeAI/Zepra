// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

namespace zepra {

enum class DownloadStatus {
    QUEUED,
    DOWNLOADING,
    PAUSED,
    COMPLETED,
    FAILED,
    CANCELED
};

struct DownloadPart {
    int partIndex;
    size_t startByte;
    size_t endByte;
    size_t downloadedBytes;
    DownloadStatus status;
    std::string tempFilePath;
};

class DownloadTask {
public:
    DownloadTask(const std::string& url, const std::string& destPath, int numParts = 5);
    ~DownloadTask();

    void start();
    void pause();
    void resume();
    void cancel();
    DownloadStatus getStatus() const;
    double getProgress() const; // 0.0 - 1.0
    double getSpeed() const;    // bytes/sec
    double getETA() const;      // seconds
    std::string getFileName() const;
    std::string getDestPath() const;
    std::string getUrl() const;
    size_t getTotalSize() const;
    bool isVideoFile() const;

    // UI callbacks
    void setProgressCallback(std::function<void(double)> cb);
    void setStatusCallback(std::function<void(DownloadStatus)> cb);

private:
    std::string url;
    std::string destPath;
    std::string fileName;
    size_t totalSize;
    int numParts;
    std::vector<DownloadPart> parts;
    std::atomic<DownloadStatus> status;
    std::function<void(double)> progressCallback;
    std::function<void(DownloadStatus)> statusCallback;
    // ... internal state ...
    void downloadPart(int partIndex);
    void mergeParts();
    void updateProgress();
    void detectFileType();
};

class DownloadManager {
public:
    DownloadManager();
    ~DownloadManager();

    // Download operations
    std::shared_ptr<DownloadTask> addDownload(const std::string& url, const std::string& destPath, int numParts = 5);
    std::shared_ptr<DownloadTask> addDownloadToDefault(const std::string& url, const std::string& filename, int numParts = 5);
    void removeDownload(const std::string& url);
    std::vector<std::shared_ptr<DownloadTask>> getAllDownloads() const;
    void setOnDownloadComplete(std::function<void(std::shared_ptr<DownloadTask>)> cb);

    // Download path configuration
    void setDefaultDownloadPath(const std::string& path);
    std::string getDefaultDownloadPath() const;
    
    // Ask before download option
    void setAskBeforeDownload(bool ask);
    bool isAskBeforeDownload() const;
    
    // Prompt callback for "ask on download" feature
    using PathPromptCallback = std::function<std::string(const std::string& suggestedPath, const std::string& filename)>;
    void setPathPromptCallback(PathPromptCallback callback);
    
    // Download with optional prompt
    std::shared_ptr<DownloadTask> startDownload(const std::string& url, const std::string& filename, int numParts = 5);

private:
    std::vector<std::shared_ptr<DownloadTask>> downloads;
    std::function<void(std::shared_ptr<DownloadTask>)> onDownloadComplete;
    std::string defaultDownloadPath_;
    bool askBeforeDownload_;
    PathPromptCallback pathPromptCallback_;
    
    // Get system default download path
    static std::string getSystemDownloadPath();
};

} // namespace zepra 