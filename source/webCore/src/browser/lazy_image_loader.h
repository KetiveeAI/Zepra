/**
 * @file lazy_image_loader.h
 * @brief Deferred image loading for ZepraBrowser
 * 
 * Instead of blocking page render while loading all images,
 * this queues images for background loading and updates
 * LayoutBoxes when images are ready.
 */

#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>

namespace ZepraBrowser {

// Forward declaration
struct LayoutBox;

/**
 * @brief Entry for a pending image load
 */
struct PendingImage {
    LayoutBox* box;           // Target box to update
    std::string url;          // Image URL
    int priority;             // Higher = load first (viewport images get higher priority)
    int tabId;                // Tab that requested this image
    
    bool operator<(const PendingImage& other) const {
        return priority < other.priority; // Lower priority = later in queue
    }
};

/**
 * @brief Result of loading an image
 */
struct ImageResult {
    LayoutBox* box;
    uint32_t textureId;
    int width;
    int height;
    bool success;
    std::string error;
};

/**
 * @brief Lazy image loader with priority queue and viewport awareness
 * 
 * Images are loaded in background threads and results are polled
 * during the render loop to update LayoutBoxes.
 */
class LazyImageLoader {
public:
    LazyImageLoader();
    ~LazyImageLoader();
    
    /**
     * @brief Queue an image for loading
     * @param box Target LayoutBox to update when loaded
     * @param url Image URL
     * @param priority Higher = load first
     * @param tabId Tab ID for cancellation
     */
    void queueImage(LayoutBox* box, const std::string& url, int priority = 0, int tabId = -1);
    
    /**
     * @brief Queue image with viewport priority
     * @param box Target box
     * @param url Image URL
     * @param inViewport True if image is currently visible
     * @param tabId Tab ID
     */
    void queueImageWithViewport(LayoutBox* box, const std::string& url, 
                                 bool inViewport, int tabId = -1);
    
    /**
     * @brief Cancel all pending images for a tab (e.g., on navigation)
     */
    void cancelTab(int tabId);
    
    /**
     * @brief Cancel all pending images
     */
    void cancelAll();
    
    /**
     * @brief Get completed images (call from main/render thread)
     * @param results Output vector for completed images
     * @param maxResults Maximum results to return (0 = all)
     */
    void pollCompleted(std::vector<ImageResult>& results, int maxResults = 10);
    
    /**
     * @brief Check if there are pending images
     */
    bool hasPending() const { return pendingCount_ > 0; }
    
    /**
     * @brief Get count of pending images
     */
    int pendingCount() const { return pendingCount_; }
    
    /**
     * @brief Start background loading threads
     */
    void start(int numThreads = 2);
    
    /**
     * @brief Stop background loading
     */
    void stop();
    
    /**
     * @brief Set callback for GPU texture creation (must be called from main thread)
     */
    using TextureCreator = std::function<uint32_t(int width, int height, const uint8_t* pixels)>;
    void setTextureCreator(TextureCreator creator) { textureCreator_ = creator; }
    
private:
    void workerThread();
    ImageResult loadImage(const PendingImage& pending);
    
    // Thread-safe queues
    std::priority_queue<PendingImage> pendingQueue_;
    std::mutex pendingMutex_;
    
    std::vector<ImageResult> completedQueue_;
    std::mutex completedMutex_;
    
    // Raw image data waiting for texture creation (main thread only)
    struct RawImage {
        LayoutBox* box;
        std::vector<uint8_t> pixels;
        int width;
        int height;
    };
    std::vector<RawImage> rawImages_;
    std::mutex rawMutex_;
    
    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    std::atomic<int> pendingCount_{0};
    
    // Texture creator (main thread callback)
    TextureCreator textureCreator_;
    
    // URL cache to avoid duplicate loads
    std::unordered_map<std::string, uint32_t> textureCache_;
    std::mutex cacheMutex_;
};

// Global instance
extern LazyImageLoader g_lazyImageLoader;

} // namespace ZepraBrowser
