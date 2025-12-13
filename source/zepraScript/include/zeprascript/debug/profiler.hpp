#pragma once

/**
 * @file profiler.hpp
 * @brief DevTools Profiler - CPU and Memory profiling
 */

#include "../config.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <memory>

namespace Zepra::Debug {

/**
 * @brief CPU profile node
 */
struct ProfileNode {
    int id;
    std::string functionName;
    std::string sourceFile;
    int lineNumber;
    int columnNumber;
    
    // Timing
    double selfTime = 0;      // Time in this function only
    double totalTime = 0;     // Time including children
    int hitCount = 0;
    
    // Tree structure
    std::vector<int> children;
    int parent = -1;
};

/**
 * @brief CPU profile sample
 */
struct ProfileSample {
    int nodeId;
    double timestamp;
};

/**
 * @brief CPU profile result
 */
struct CPUProfile {
    std::string name;
    double startTime;
    double endTime;
    std::vector<ProfileNode> nodes;
    std::vector<ProfileSample> samples;
    
    double duration() const { return endTime - startTime; }
};

/**
 * @brief Memory snapshot node
 */
struct HeapNode {
    int id;
    std::string name;
    std::string type;      // "object", "array", "string", etc.
    size_t selfSize = 0;
    size_t retainedSize = 0;
    std::vector<int> children;
    std::vector<int> edges;
};

/**
 * @brief Memory snapshot
 */
struct HeapSnapshot {
    std::string name;
    double timestamp;
    std::vector<HeapNode> nodes;
    size_t totalSize = 0;
    size_t totalObjects = 0;
};

/**
 * @brief CPU Profiler
 */
class CPUProfiler {
public:
    /**
     * @brief Start profiling
     */
    void start(const std::string& name = "Profile");
    
    /**
     * @brief Stop profiling and get result
     */
    std::unique_ptr<CPUProfile> stop();
    
    /**
     * @brief Check if profiling
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Record function entry (called by VM)
     */
    void onFunctionEnter(const std::string& functionName, 
                          const std::string& file, int line);
    
    /**
     * @brief Record function exit (called by VM)
     */
    void onFunctionExit();
    
    /**
     * @brief Get current sample rate (samples per second)
     */
    int sampleRate() const { return sampleRateHz_; }
    void setSampleRate(int hz) { sampleRateHz_ = hz; }
    
private:
    void takeSample();
    int getOrCreateNode(const std::string& name, const std::string& file, int line);
    
    bool running_ = false;
    int sampleRateHz_ = 1000;
    std::string profileName_;
    
    std::chrono::steady_clock::time_point startTime_;
    std::vector<ProfileNode> nodes_;
    std::vector<ProfileSample> samples_;
    std::vector<int> callStack_;
    
    std::unordered_map<std::string, int> nodeMap_;
    int nextNodeId_ = 0;
};

/**
 * @brief Memory Profiler
 */
class MemoryProfiler {
public:
    /**
     * @brief Take heap snapshot
     */
    std::unique_ptr<HeapSnapshot> takeSnapshot(const std::string& name = "Snapshot");
    
    /**
     * @brief Start tracking allocations
     */
    void startTrackingAllocations();
    
    /**
     * @brief Stop tracking allocations
     */
    std::vector<std::pair<std::string, size_t>> stopTrackingAllocations();
    
    /**
     * @brief Get current memory usage
     */
    size_t getUsedHeapSize() const;
    size_t getTotalHeapSize() const;
    
    /**
     * @brief Record allocation (called by GC)
     */
    void onAllocation(void* ptr, size_t size, const std::string& type);
    
    /**
     * @brief Record deallocation (called by GC)
     */
    void onDeallocation(void* ptr);
    
private:
    bool trackingAllocations_ = false;
    std::unordered_map<void*, std::pair<size_t, std::string>> allocations_;
    std::vector<std::pair<std::string, size_t>> trackedAllocations_;
};

/**
 * @brief Performance timeline event
 */
struct TimelineEvent {
    enum class Type {
        Script,
        Style,
        Layout,
        Paint,
        Composite,
        GC,
        Network,
        Timer
    } type;
    
    std::string name;
    double startTime;
    double endTime;
    std::string details;
    
    double duration() const { return endTime - startTime; }
};

/**
 * @brief Performance Timeline recorder
 */
class Timeline {
public:
    void start();
    void stop();
    bool isRecording() const { return recording_; }
    
    void addEvent(const TimelineEvent& event);
    std::vector<TimelineEvent> getEvents() const { return events_; }
    void clearEvents() { events_.clear(); }
    
private:
    bool recording_ = false;
    std::vector<TimelineEvent> events_;
};

} // namespace Zepra::Debug
