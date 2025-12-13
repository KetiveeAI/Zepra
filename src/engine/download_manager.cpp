#include "../../include/engine/download_manager.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <chrono>
#include <curl/curl.h>

namespace zepra {

DownloadTask::DownloadTask(const std::string& url, const std::string& destPath, int numParts)
    : url(url), destPath(destPath), numParts(numParts), status(DownloadStatus::QUEUED), totalSize(0) {
    // TODO: Query file size, split into parts
    // For now, just create empty parts
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
    // TODO: Query file size, split into parts, start threads
    // For now, just simulate download
    std::thread([this]() {
        for (int i = 0; i < numParts; ++i) {
            downloadPart(i);
        }
        mergeParts();
        status = DownloadStatus::COMPLETED;
        if (statusCallback) statusCallback(status);
    }).detach();
}

void DownloadTask::pause() {
    status = DownloadStatus::PAUSED;
    std::cout << "[DownloadTask] Paused" << std::endl;
    if (statusCallback) statusCallback(status);
}

void DownloadTask::resume() {
    status = DownloadStatus::DOWNLOADING;
    std::cout << "[DownloadTask] Resumed" << std::endl;
    if (statusCallback) statusCallback(status);
}

void DownloadTask::cancel() {
    status = DownloadStatus::CANCELED;
    std::cout << "[DownloadTask] Canceled" << std::endl;
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
bool DownloadTask::isVideoFile() const { return fileName.find(",mp4") != std::string::npos || fileName.find(".mkv") != std::string::npos || fileName.find(".webm") != std::string::npos; }
void DownloadTask::setProgressCallback(std::function<void(double)> cb) { progressCallback = cb; }
void DownloadTask::setStatusCallback(std::function<void(DownloadStatus)> cb) { statusCallback = cb; }
void DownloadTask::downloadPart(int partIndex) {
    // TODO: Use libcurl to download part with HTTP range
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

DownloadManager::DownloadManager() {}
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

void DownloadManager::removeDownload(const std::string& url) {
    downloads.erase(std::remove_if(downloads.begin(), downloads.end(), [&](const std::shared_ptr<DownloadTask>& t) {
        return t->getUrl() == url;
    }), downloads.end());
}

std::vector<std::shared_ptr<DownloadTask>> DownloadManager::getAllDownloads() const { return downloads; }
void DownloadManager::setOnDownloadComplete(std::function<void(std::shared_ptr<DownloadTask>)> cb) { onDownloadComplete = cb; }

} // namespace zepra 