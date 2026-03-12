// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file lazy_image_loader.cpp
 * @brief Implementation of deferred image loading
 */

#include "lazy_image_loader.h"
#include "layout_engine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>
// HTTP client for fetching images
#include "networking/http_client.hpp"

// Image decoding (stb_image is already included in main file, just declare here)
extern "C" {
    unsigned char* stbi_load_from_memory(const unsigned char* buffer, int len, 
                                          int* x, int* y, int* channels_in_file, 
                                          int desired_channels);
    void stbi_image_free(void* retval_from_stbi_load);
    const char* stbi_failure_reason(void);
}

namespace ZepraBrowser {

// Global instance
LazyImageLoader g_lazyImageLoader;

LazyImageLoader::LazyImageLoader() {
    std::cout << "[LazyImageLoader] Initialized" << std::endl;
}

LazyImageLoader::~LazyImageLoader() {
    stop();
}

void LazyImageLoader::start(int numThreads) {
    if (running_) return;
    
    running_ = true;
    
    for (int i = 0; i < numThreads; i++) {
        workers_.emplace_back(&LazyImageLoader::workerThread, this);
    }
    
    std::cout << "[LazyImageLoader] Started " << numThreads << " worker threads" << std::endl;
}

void LazyImageLoader::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // Wake up all workers
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        // Clear queue
        while (!pendingQueue_.empty()) pendingQueue_.pop();
    }
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    
    std::cout << "[LazyImageLoader] Stopped" << std::endl;
}

void LazyImageLoader::queueImage(LayoutBox* box, const std::string& url, 
                                  int priority, int tabId) {
    if (!box || url.empty()) return;
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = textureCache_.find(url);
        if (it != textureCache_.end()) {
            // Already loaded - update box directly
            box->textureId = it->second;
            box->text = ""; // Clear placeholder
            std::cout << "[LazyImageLoader] Cache hit: " << url.substr(url.rfind('/') + 1) << std::endl;
            return;
        }
    }
    
    PendingImage pending;
    pending.box = box;
    pending.url = url;
    pending.priority = priority;
    pending.tabId = tabId;
    
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingQueue_.push(pending);
        pendingCount_++;
    }
    
    // Set placeholder
    box->text = "[IMG: Loading...]";
    box->color = 0xAAAAAA;
}

void LazyImageLoader::queueImageWithViewport(LayoutBox* box, const std::string& url, 
                                              bool inViewport, int tabId) {
    // Viewport images get priority 100, others get 0
    int priority = inViewport ? 100 : 0;
    queueImage(box, url, priority, tabId);
}

void LazyImageLoader::cancelTab(int tabId) {
    // We can't easily remove from priority_queue, so we'll mark entries
    // and skip them in worker. For now, just clear all.
    // TODO: Implement proper per-tab cancellation
    std::cout << "[LazyImageLoader] Cancel requested for tab " << tabId << std::endl;
}

void LazyImageLoader::cancelAll() {
    std::lock_guard<std::mutex> lock(pendingMutex_);
    while (!pendingQueue_.empty()) {
        pendingQueue_.pop();
    }
    pendingCount_ = 0;
    std::cout << "[LazyImageLoader] All pending images cancelled" << std::endl;
}

void LazyImageLoader::pollCompleted(std::vector<ImageResult>& results, int maxResults) {
    // First, process raw images that need texture creation (main thread only)
    {
        std::lock_guard<std::mutex> lock(rawMutex_);
        for (auto& raw : rawImages_) {
            // Guard against stale box pointer
            if (!raw.box) continue;
            
            if (textureCreator_ && !raw.pixels.empty()) {
                uint32_t texId = textureCreator_(raw.width, raw.height, raw.pixels.data());
                
                if (texId > 0 && raw.box) {
                    raw.box->textureId = texId;
                    raw.box->width = (float)raw.width;
                    raw.box->height = (float)raw.height;
                    raw.box->text = ""; // Clear placeholder
                    
                    std::cout << "[LazyImageLoader] Texture created: " << raw.width 
                              << "x" << raw.height << std::endl;
                } else if (raw.box) {
                    raw.box->text = "[IMG: GPU Error]";
                    raw.box->color = 0xFF0000;
                }
            }
        }
        rawImages_.clear();
    }
    
    // Then return completed results
    std::lock_guard<std::mutex> lock(completedMutex_);
    int count = maxResults > 0 ? std::min(maxResults, (int)completedQueue_.size()) 
                               : (int)completedQueue_.size();
    
    for (int i = 0; i < count; i++) {
        results.push_back(completedQueue_.back());
        completedQueue_.pop_back();
    }
}

void LazyImageLoader::workerThread() {
    std::cout << "[LazyImageLoader] Worker thread started" << std::endl;
    
    while (running_) {
        PendingImage pending;
        bool hasPending = false;
        
        // Get next image from queue
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            if (!pendingQueue_.empty()) {
                pending = pendingQueue_.top();
                pendingQueue_.pop();
                pendingCount_--;
                hasPending = true;
            }
        }
        
        if (hasPending) {
            // Load the image
            ImageResult result = loadImage(pending);
            
            // Add to completed queue
            {
                std::lock_guard<std::mutex> lock(completedMutex_);
                completedQueue_.push_back(result);
            }
        } else {
            // No work - sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    std::cout << "[LazyImageLoader] Worker thread stopped" << std::endl;
}

ImageResult LazyImageLoader::loadImage(const PendingImage& pending) {
    ImageResult result;
    result.box = pending.box;
    result.success = false;
    result.textureId = 0;
    result.width = 0;
    result.height = 0;
    
    // Guard against stale box pointer (can happen after page navigation/repaint)
    if (!pending.box) {
        result.error = "Box pointer is null";
        return result;
    }
    
    try {
        // Fetch image data
        Zepra::Networking::HttpClientConfig config;
        config.connectTimeoutMs = 10000;
        config.readTimeoutMs = 30000;
        config.followRedirects = true;
        config.maxRedirects = 10;
        config.verifySsl = false;
        config.userAgent = "ZepraBrowser/1.0 (NeolyxOS)";
        
        Zepra::Networking::HttpClient client(config);
        Zepra::Networking::HttpRequest request(Zepra::Networking::HttpMethod::GET, pending.url);
        
        auto response = client.send(request);
        
        if (!response.isSuccess()) {
            result.error = response.error();
            if (pending.box) {
                pending.box->text = "[IMG: Load Failed]";
                pending.box->color = 0xFF0000;
            }
            return result;
        }
        
        const auto& data = response.body();
        if (data.empty()) {
            result.error = "Empty response";
            if (pending.box) {
                pending.box->text = "[IMG: Empty]";
                pending.box->color = 0xFF0000;
            }
            return result;
        }
        // =====================================================================
        // Image Format Detection (Following Firefox's DecoderFactory pattern)
        // Tier 1: Content-Type header (most reliable)
        // Tier 2: URL extension fallback
        // Tier 3: Magic bytes verification
        // =====================================================================
        
        bool isSvg = false;
        bool isRaster = false;
        std::string detectedFormat = "unknown";
        
        // Tier 1: Content-Type header (most reliable - Firefox primary method)
        std::string contentType = response.header("Content-Type");
        std::transform(contentType.begin(), contentType.end(), contentType.begin(), ::tolower);
        
        if (contentType.find("image/svg") != std::string::npos ||
            contentType.find("svg+xml") != std::string::npos) {
            isSvg = true;
            detectedFormat = "svg";
        } else if (contentType.find("image/png") != std::string::npos) {
            isRaster = true;
            detectedFormat = "png";
        } else if (contentType.find("image/jpeg") != std::string::npos ||
                   contentType.find("image/jpg") != std::string::npos) {
            isRaster = true;
            detectedFormat = "jpeg";
        } else if (contentType.find("image/gif") != std::string::npos) {
            isRaster = true;
            detectedFormat = "gif";
        } else if (contentType.find("image/webp") != std::string::npos) {
            isRaster = true;
            detectedFormat = "webp";
        } else if (contentType.find("image/bmp") != std::string::npos) {
            isRaster = true;
            detectedFormat = "bmp";
        } else if (contentType.find("image/") != std::string::npos) {
            // Generic image type - let stb_image try to decode
            isRaster = true;
            detectedFormat = "generic-image";
        }
        
        // Tier 2: URL extension fallback (when Content-Type is missing or generic)
        if (!isSvg && !isRaster) {
            std::string urlLower = pending.url;
            std::transform(urlLower.begin(), urlLower.end(), urlLower.begin(), ::tolower);
            
            // Remove query params for extension check
            size_t queryPos = urlLower.find('?');
            if (queryPos != std::string::npos) {
                urlLower = urlLower.substr(0, queryPos);
            }
            
            if (urlLower.rfind(".svg") == urlLower.length() - 4) {
                isSvg = true;
                detectedFormat = "svg-url";
            } else if (urlLower.rfind(".png") == urlLower.length() - 4) {
                isRaster = true;
                detectedFormat = "png-url";
            } else if (urlLower.rfind(".jpg") == urlLower.length() - 4 ||
                       urlLower.rfind(".jpeg") == urlLower.length() - 5) {
                isRaster = true;
                detectedFormat = "jpeg-url";
            } else if (urlLower.rfind(".gif") == urlLower.length() - 4) {
                isRaster = true;
                detectedFormat = "gif-url";
            } else if (urlLower.rfind(".webp") == urlLower.length() - 5) {
                isRaster = true;
                detectedFormat = "webp-url";
            } else if (urlLower.rfind(".bmp") == urlLower.length() - 4) {
                isRaster = true;
                detectedFormat = "bmp-url";
            }
        }
        
        // Tier 3: Magic bytes detection (content sniffing - last resort)
        if (!isSvg && !isRaster && data.size() > 16) {
            const uint8_t* bytes = data.data();
            
            // PNG: 89 50 4E 47 0D 0A 1A 0A
            if (bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47) {
                isRaster = true;
                detectedFormat = "png-magic";
            }
            // JPEG: FF D8 FF
            else if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
                isRaster = true;
                detectedFormat = "jpeg-magic";
            }
            // GIF: GIF89a or GIF87a
            else if (bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' &&
                     bytes[3] == '8' && (bytes[4] == '9' || bytes[4] == '7') && bytes[5] == 'a') {
                isRaster = true;
                detectedFormat = "gif-magic";
            }
            // WebP: RIFF....WEBP
            else if (bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
                     data.size() > 12 && bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P') {
                isRaster = true;
                detectedFormat = "webp-magic";
            }
            // BMP: BM
            else if (bytes[0] == 'B' && bytes[1] == 'M') {
                isRaster = true;
                detectedFormat = "bmp-magic";
            }
            // SVG: Check XML/SVG markers at start
            else if (data.size() > 5) {
                const char* content = (const char*)bytes;
                if (strncmp(content, "<?xml", 5) == 0 ||
                    strncmp(content, "<svg", 4) == 0 ||
                    (data.size() > 15 && strncmp(content, "<!DOCTYPE svg", 13) == 0)) {
                    isSvg = true;
                    detectedFormat = "svg-magic";
                } else if (data.size() > 100) {
                    // Deep search for SVG in first 100 bytes
                    std::string firstBytes(content, std::min((size_t)100, data.size()));
                    if (firstBytes.find("<svg") != std::string::npos) {
                        isSvg = true;
                        detectedFormat = "svg-deep";
                    }
                }
            }
        }
        
        // Debug logging
        std::cout << "[LazyImageLoader] Format detected: " << detectedFormat 
                  << " (CT: " << (contentType.empty() ? "none" : contentType.substr(0, 30)) << ")" << std::endl;
        
        // Handle SVG (placeholder - needs NxSVG on main thread)
        if (isSvg) {
            if (pending.box) {
                pending.box->text = "";  // Clear loading text
                pending.box->width = 24;
                pending.box->height = 24;
                pending.box->color = 0x0066CC;  // Blue icon color
                pending.box->bgColor = 0xE6F0FF;
                pending.box->hasBgColor = true;
            }
            result.success = true;
            result.width = 24;
            result.height = 24;
            return result;
        }
        
        // Unknown format - show warning instead of trying to decode
        if (!isRaster) {
            result.error = "Unknown image format (" + detectedFormat + ")";
            if (pending.box) {
                pending.box->text = "[IMG: Unknown]";
                pending.box->color = 0xFF6600; // Orange for warnings
            }
            std::cout << "[LazyImageLoader] Skipping unknown format: " << pending.url.substr(0, 80) << std::endl;
            return result;
        }
        
        // Decode raster image (PNG, JPG, etc.)
        int w, h, channels;
        unsigned char* pixels = stbi_load_from_memory(data.data(), (int)data.size(), 
                                                       &w, &h, &channels, 4); // Force RGBA
        
        if (!pixels) {
            result.error = stbi_failure_reason();
            if (pending.box) {
                pending.box->text = "[IMG: Decode Failed]";
                pending.box->color = 0xFF0000;
            }
            return result;
        }
        
        result.width = w;
        result.height = h;
        result.success = true;
        
        // Queue raw pixels for main thread texture creation
        {
            std::lock_guard<std::mutex> lock(rawMutex_);
            RawImage raw;
            raw.box = pending.box;
            raw.width = w;
            raw.height = h;
            raw.pixels.assign(pixels, pixels + (w * h * 4));
            rawImages_.push_back(std::move(raw));
        }
        
        stbi_image_free(pixels);
        
        std::cout << "[LazyImageLoader] Loaded: " << pending.url.substr(pending.url.rfind('/') + 1) 
                  << " (" << w << "x" << h << ")" << std::endl;
        
    } catch (const std::exception& e) {
        result.error = e.what();
        if (pending.box) {
            pending.box->text = "[IMG: Error]";
            pending.box->color = 0xFF0000;
        }
    }
    
    return result;
}

} // namespace ZepraBrowser
