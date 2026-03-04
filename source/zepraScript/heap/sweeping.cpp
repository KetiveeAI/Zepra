/**
 * @file sweeping.cpp
 * @brief Concurrent sweeping implementation for GC
 * 
 * Implements the sweeping phase of garbage collection which reclaims
 * memory from unmarked (dead) objects. Supports concurrent sweeping
 * to reduce pause times.
 */

#include "heap/heap.hpp"
#include "runtime/objects/object.hpp"
#include <vector>
#include <atomic>
#include <chrono>
#include <cassert>
#include <thread>

namespace Zepra::GC {

// =============================================================================
// SweepResult - Statistics from a sweep operation
// =============================================================================

struct SweepResult {
    size_t objectsSwept = 0;
    size_t bytesReclaimed = 0;
    size_t objectsSurvived = 0;
    size_t bytesSurvived = 0;
    double sweepTimeMs = 0;
};

// =============================================================================
// FinalizeCallback - For C++ destructor integration
// =============================================================================

/**
 * @brief Finalization callback type
 * Called before an object is reclaimed, allowing cleanup
 */
using FinalizeCallback = std::function<void(Runtime::Object*)>;

// =============================================================================
// SweepPage - Represents a page of objects to sweep
// =============================================================================

struct SweepPage {
    std::vector<Runtime::Object*> objects;
    size_t sweepIndex = 0;
    bool complete = false;
    
    SweepPage() = default;
    
    explicit SweepPage(std::vector<Runtime::Object*>&& objs)
        : objects(std::move(objs)), sweepIndex(0), complete(false) {}
    
    bool empty() const { return objects.empty(); }
    
    size_t remaining() const {
        return complete ? 0 : (objects.size() - sweepIndex);
    }
};

// =============================================================================
// Sweeper - Manages the sweeping process
// =============================================================================

class Sweeper {
public:
    explicit Sweeper(Heap* heap) : heap_(heap) {}
    
    /**
     * @brief Prepare for sweeping
     * Called after marking is complete
     */
    void prepare(std::vector<void*>& objects) {
        sweepPages_.clear();
        currentPage_ = 0;
        totalResult_ = SweepResult{};
        
        // Convert void* to Object* and organize into pages
        std::vector<Runtime::Object*> page;
        page.reserve(PAGE_SIZE);
        
        for (void* ptr : objects) {
            page.push_back(static_cast<Runtime::Object*>(ptr));
            
            if (page.size() >= PAGE_SIZE) {
                sweepPages_.emplace_back(std::move(page));
                page = std::vector<Runtime::Object*>();
                page.reserve(PAGE_SIZE);
            }
        }
        
        // Add remaining objects as final page
        if (!page.empty()) {
            sweepPages_.emplace_back(std::move(page));
        }
    }
    
    /**
     * @brief Perform incremental sweeping
     * @param budget Maximum number of objects to sweep
     * @param deadObjects Output: objects that were not marked (to be freed)
     * @return true if sweeping is complete
     */
    bool sweepIncremental(size_t budget, std::vector<Runtime::Object*>& deadObjects) {
        size_t processed = 0;
        
        while (currentPage_ < sweepPages_.size() && processed < budget) {
            SweepPage& page = sweepPages_[currentPage_];
            
            while (page.sweepIndex < page.objects.size() && processed < budget) {
                Runtime::Object* obj = page.objects[page.sweepIndex++];
                
                if (!obj) continue;
                
                if (obj->isMarked()) {
                    // Object survived - clear mark for next GC cycle
                    obj->clearMark();
                    totalResult_.objectsSurvived++;
                    totalResult_.bytesSurvived += sizeof(Runtime::Object);
                } else {
                    // Object is dead - add to dead list
                    deadObjects.push_back(obj);
                    totalResult_.objectsSwept++;
                    totalResult_.bytesReclaimed += sizeof(Runtime::Object);
                }
                
                processed++;
            }
            
            if (page.sweepIndex >= page.objects.size()) {
                page.complete = true;
                currentPage_++;
            }
        }
        
        return currentPage_ >= sweepPages_.size();
    }
    
    /**
     * @brief Sweep all remaining objects at once
     * @param deadObjects Output: objects that were not marked
     */
    void sweepComplete(std::vector<Runtime::Object*>& deadObjects) {
        while (!isComplete()) {
            sweepIncremental(SIZE_MAX, deadObjects);
        }
    }
    
    /**
     * @brief Finalize and delete dead objects
     */
    static void finalizeObjects(std::vector<Runtime::Object*>& deadObjects,
                                 FinalizeCallback finalizer = nullptr) {
        for (Runtime::Object* obj : deadObjects) {
            if (!obj) continue;
            
            // Call finalizer if provided
            if (finalizer) {
                finalizer(obj);
            }
            
            // Delete the object
            delete obj;
        }
        
        deadObjects.clear();
    }
    
    /**
     * @brief Check if sweeping is complete
     */
    bool isComplete() const {
        return currentPage_ >= sweepPages_.size();
    }
    
    /**
     * @brief Get sweep result statistics
     */
    const SweepResult& result() const { return totalResult_; }
    
    /**
     * @brief Get progress (0.0 - 1.0)
     */
    double progress() const {
        if (sweepPages_.empty()) return 1.0;
        
        size_t totalObjects = 0;
        size_t processedObjects = 0;
        
        for (size_t i = 0; i < sweepPages_.size(); ++i) {
            totalObjects += sweepPages_[i].objects.size();
            if (i < currentPage_) {
                processedObjects += sweepPages_[i].objects.size();
            } else if (i == currentPage_) {
                processedObjects += sweepPages_[i].sweepIndex;
            }
        }
        
        return totalObjects > 0 ? 
            static_cast<double>(processedObjects) / totalObjects : 1.0;
    }
    
    /**
     * @brief Reset sweeper state
     */
    void reset() {
        sweepPages_.clear();
        currentPage_ = 0;
        totalResult_ = SweepResult{};
    }
    
private:
    static constexpr size_t PAGE_SIZE = 256;
    
    Heap* heap_;
    std::vector<SweepPage> sweepPages_;
    size_t currentPage_ = 0;
    SweepResult totalResult_;
};

// =============================================================================
// LargeObjectSweeper - Specialized sweeper for large objects
// =============================================================================

class LargeObjectSweeper {
public:
    struct LargeObject {
        void* ptr;
        size_t size;
        bool marked;
    };
    
    /**
     * @brief Sweep large objects list
     * @return Bytes reclaimed
     */
    static size_t sweep(std::vector<LargeObject>& objects, 
                        FinalizeCallback finalizer = nullptr) {
        size_t bytesReclaimed = 0;
        
        auto newEnd = std::remove_if(objects.begin(), objects.end(),
            [&](LargeObject& obj) {
                if (!obj.marked) {
                    // Call finalizer if provided
                    if (finalizer && obj.ptr) {
                        Runtime::Object* runtimeObj = 
                            static_cast<Runtime::Object*>(obj.ptr);
                        finalizer(runtimeObj);
                    }
                    
                    // Free memory
                    std::free(obj.ptr);
                    bytesReclaimed += obj.size;
                    return true;  // Remove from list
                }
                
                // Clear mark for next cycle
                obj.marked = false;
                return false;  // Keep in list
            });
        
        objects.erase(newEnd, objects.end());
        return bytesReclaimed;
    }
};

// =============================================================================
// Global Sweeper Instance
// =============================================================================

static Sweeper* globalSweeper = nullptr;
static SweepResult globalSweepStats;

void initializeSweeper(Heap* heap) {
    if (!globalSweeper) {
        globalSweeper = new Sweeper(heap);
    }
}

void shutdownSweeper() {
    delete globalSweeper;
    globalSweeper = nullptr;
}

Sweeper* getSweeper() {
    return globalSweeper;
}

const SweepResult& getSweepStats() {
    return globalSweepStats;
}

// =============================================================================
// Concurrent Sweeping Support (Future Enhancement)
// =============================================================================

/**
 * @brief Configuration for concurrent sweeping
 */
struct ConcurrentSweepConfig {
    bool enabled = false;
    size_t numWorkers = 1;
    size_t pagesPerWorker = 4;
};

/**
 * @brief Concurrent sweep task (placeholder for future implementation)
 * 
 * In a full implementation, this would:
 * 1. Divide sweep pages among worker threads
 * 2. Each worker sweeps its pages independently
 * 3. Synchronize dead object collection at the end
 * 4. Main thread finalizes and frees objects
 */
class ConcurrentSweepTask {
public:
    ConcurrentSweepTask(const ConcurrentSweepConfig& config)
        : config_(config) {}
    
    void execute(std::vector<SweepPage>& pages) {
        if (pages.empty() || !config_.enabled || config_.numWorkers <= 1) {
            // Fallback to sequential sweep
            for (auto& page : pages) {
                sweepPage(page, deadObjects_);
            }
            return;
        }
        
        size_t numWorkers = std::min(config_.numWorkers, pages.size());
        std::vector<std::thread> workers;
        std::vector<std::vector<Runtime::Object*>> perThreadDead(numWorkers);
        
        // Divide pages among workers
        size_t pagesPerWorker = (pages.size() + numWorkers - 1) / numWorkers;
        
        for (size_t w = 0; w < numWorkers; ++w) {
            size_t start = w * pagesPerWorker;
            size_t end = std::min(start + pagesPerWorker, pages.size());
            if (start >= pages.size()) break;
            
            workers.emplace_back([&pages, &perThreadDead, start, end, w]() {
                for (size_t i = start; i < end; ++i) {
                    sweepPage(pages[i], perThreadDead[w]);
                }
            });
        }
        
        // Wait for all workers
        for (auto& t : workers) {
            t.join();
        }
        
        // Merge per-thread dead lists
        for (auto& threadDead : perThreadDead) {
            deadObjects_.insert(deadObjects_.end(), 
                threadDead.begin(), threadDead.end());
        }
    }
    
    std::vector<Runtime::Object*>& deadObjects() { return deadObjects_; }
    
private:
    static void sweepPage(SweepPage& page, std::vector<Runtime::Object*>& dead) {
        for (auto* obj : page.objects) {
            if (!obj) continue;
            if (obj->isMarked()) {
                obj->clearMark();
            } else {
                dead.push_back(obj);
            }
        }
        page.complete = true;
    }
    
    ConcurrentSweepConfig config_;
    std::vector<Runtime::Object*> deadObjects_;
};

} // namespace Zepra::GC
