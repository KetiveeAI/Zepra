// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/download_manager.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <chrono>
#include <nxhttp.h>
#include <cstdlib>
#include <sys/stat.h>

namespace zepra {

// Get system default download path (~/Downloads)
std::string DownloadManager::getSystemDownloadPath() {
    const char* home = std::getenv("HOME");
    if (home) {
        std::string downloadPath = std::string(home) + "/Downloads";
        
        // Create directory if it doesn't exist
        struct stat st;
        if (stat(downloadPath.c_str(), &st) != 0) {
            mkdir(downloadPath.c_str(), 0755);
        }
        
        return downloadPath;
    }
    return "/tmp";  // Fallback
}

DownloadTask::DownloadTask(const std::string& url, const std::string& destPath, int numParts)
    : url(url), destPath(destPath), numParts(numParts), status(DownloadStatus::QUEUED), totalSize(0) {
    for (int i = 0; i < numParts; ++i) {
        DownloadPart part;
        part.partIndex = i;
        part.startByte = 0;
        part.endByte = 0;
        part.downloadedBytes = 0;
        part.status = DownloadStatus::QUEUED;
        part.tempFilePath = destPath + ".part" + std::to_string(i);
        parts.push_back(part);
    }
    fileName = destPath.substr(destPath.find_last_of("/\\") + 1);
    detectFileType();
}

DownloadTask::~DownloadTask() {}

void DownloadTask::start() {
    status = DownloadStatus::DOWNLOADING;
    std::cout << "[DownloadTask] Starting download: " << url << std::endl;
    std::cout << "[DownloadTask] Destination: " << destPath << std::endl;
    
    std::thread([this]() {
        for (int i = 0; i < numParts; ++i) {
            downloadPart(i);
        }
        mergeParts();
        status = DownloadStatus::COMPLETED;
        std::cout << "[DownloadTask] Download completed: " << fileName << std::endl;
        if (statusCallback) statusCallback(status);
    }).detach();
}

void DownloadTask::pause() {
    status = DownloadStatus::PAUSED;
    std::cout << "[DownloadTask] Paused: " << fileName << std::endl;
    if (statusCallback) statusCallback(status);
}

void DownloadTask::resume() {
    status = DownloadStatus::DOWNLOADING;
    std::cout << "[DownloadTask] Resumed: " << fileName << std::endl;
    if (statusCallback) statusCallback(status);
}

void DownloadTask::cancel() {
    status = DownloadStatus::CANCELED;
    std::cout << "[DownloadTask] Canceled: " << fileName << std::endl;
    if (statusCallback) statusCallback(status);
}

DownloadStatus DownloadTask::getStatus() const { return status; }
double DownloadTask::getProgress() const { return 0.0; } // TODO
double DownloadTask::getSpeed() const { return 0.0; }    // TODO
double DownloadTask::getETA() const { return 0.0; }      // TODO
std::string DownloadTask::getFileName() const { return fileName; }
std::string DownloadTask::getDestPath() const { return destPath; }
std::string DownloadTask::getUrl() const { return url; }
size_t DownloadTask::getTotalSize() const { return totalSize; }
bool DownloadTask::isVideoFile() const { 
    return fileName.find(".mp4") != std::string::npos || 
           fileName.find(".mkv") != std::string::npos || 
           fileName.find(".webm") != std::string::npos; 
}

void DownloadTask::setProgressCallback(std::function<void(double)> cb) { progressCallback = cb; }
void DownloadTask::setStatusCallback(std::function<void(DownloadStatus)> cb) { statusCallback = cb; }

void DownloadTask::downloadPart(int partIndex) {
    // TODO: Use nxhttp to download part with HTTP range headers
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate
    parts[partIndex].status = DownloadStatus::COMPLETED;
    updateProgress();
}

void DownloadTask::mergeParts() {
    std::cout << "[DownloadTask] Merging parts..." << std::endl;
    // TODO: Merge part files into destPath
}

void DownloadTask::updateProgress() {
    // TODO: Calculate and call progressCallback
}

void DownloadTask::detectFileType() {
    // TODO: Detect if file is video
}

// DownloadManager implementation

DownloadManager::DownloadManager() 
    : defaultDownloadPath_(getSystemDownloadPath())
    , askBeforeDownload_(false) 
{
    std::cout << "[DownloadManager] Initialized with default path: " << defaultDownloadPath_ << std::endl;
}

DownloadManager::~DownloadManager() {}

std::shared_ptr<DownloadTask> DownloadManager::addDownload(const std::string& url, const std::string& destPath, int numParts) {
    auto task = std::make_shared<DownloadTask>(url, destPath, numParts);
    downloads.push_back(task);
    task->setStatusCallback([this, task](DownloadStatus status) {
        if (status == DownloadStatus::COMPLETED && onDownloadComplete) {
            onDownloadComplete(task);
        }
    });
    task->start();
    return task;
}

std::shared_ptr<DownloadTask> DownloadManager::addDownloadToDefault(const std::string& url, const std::string& filename, int numParts) {
    std::string fullPath = defaultDownloadPath_ + "/" + filename;
    return addDownload(url, fullPath, numParts);
}

std::shared_ptr<DownloadTask> DownloadManager::startDownload(const std::string& url, const std::string& filename, int numParts) {
    std::string destPath;
    
    if (askBeforeDownload_ && pathPromptCallback_) {
        // Ask user for download path
        std::string suggestedPath = defaultDownloadPath_ + "/" + filename;
        destPath = pathPromptCallback_(suggestedPath, filename);
        
        if (destPath.empty()) {
            // User cancelled
            std::cout << "[DownloadManager] Download cancelled by user" << std::endl;
            return nullptr;
        }
    } else {
        // Use default path
        destPath = defaultDownloadPath_ + "/" + filename;
    }
    
    return addDownload(url, destPath, numParts);
}

void DownloadManager::removeDownload(const std::string& url) {
    downloads.erase(std::remove_if(downloads.begin(), downloads.end(), [&](const std::shared_ptr<DownloadTask>& t) {
        return t->getUrl() == url;
    }), downloads.end());
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::getAllDownloads() const { 
    return downloads; 
}

void DownloadManager::setOnDownloadComplete(std::function<void(std::shared_ptr<DownloadTask>)> cb) { 
    onDownloadComplete = cb; 
}

void DownloadManager::setDefaultDownloadPath(const std::string& path) {
    defaultDownloadPath_ = path;
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        mkdir(path.c_str(), 0755);
    }
    
    std::cout << "[DownloadManager] Default path set to: " << path << std::endl;
}

std::string DownloadManager::getDefaultDownloadPath() const {
    return defaultDownloadPath_;
}

void DownloadManager::setAskBeforeDownload(bool ask) {
    askBeforeDownload_ = ask;
    std::cout << "[DownloadManager] Ask before download: " << (ask ? "ON" : "OFF") << std::endl;
}

bool DownloadManager::isAskBeforeDownload() const {
    return askBeforeDownload_;
}

void DownloadManager::setPathPromptCallback(PathPromptCallback callback) {
    pathPromptCallback_ = callback;
}

} // namespace zepra