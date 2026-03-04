/**
 * @file cpu_profiler.cpp
 * @brief Sampling CPU profiler
 *
 * Periodically samples the call stack to build a statistical
 * profile of where CPU time is spent.
 *
 * Uses a separate sampling thread that fires at a configurable
 * interval (default: 1ms). Each sample records the current call stack.
 * After profiling, the samples are aggregated into a tree.
 *
 * Ref: V8 CpuProfiler, perf(1), gperftools
 */

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>

namespace Zepra::Profiler {

// =============================================================================
// Call Frame (for profiler)
// =============================================================================

struct ProfileFrame {
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;

    bool operator==(const ProfileFrame& other) const {
        return functionName == other.functionName &&
               scriptUrl == other.scriptUrl &&
               lineNumber == other.lineNumber;
    }
};

struct ProfileFrameHash {
    size_t operator()(const ProfileFrame& f) const {
        size_t h = std::hash<std::string>{}(f.functionName);
        h ^= std::hash<std::string>{}(f.scriptUrl) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(f.lineNumber) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// =============================================================================
// Profile Node (tree representation)
// =============================================================================

struct ProfileNode {
    uint32_t id;
    ProfileFrame frame;
    uint64_t selfSamples = 0;    // Samples where this was top of stack
    uint64_t totalSamples = 0;   // Samples that pass through this node
    std::vector<uint32_t> children;

    double selfTimeMs(uint32_t samplingIntervalUs) const {
        return selfSamples * samplingIntervalUs / 1000.0;
    }

    double totalTimeMs(uint32_t samplingIntervalUs) const {
        return totalSamples * samplingIntervalUs / 1000.0;
    }
};

// =============================================================================
// Profile Result
// =============================================================================

struct ProfileResult {
    std::vector<ProfileNode> nodes;
    uint64_t startTimeUs = 0;
    uint64_t endTimeUs = 0;
    uint32_t totalSamples = 0;
    uint32_t samplingIntervalUs = 1000;

    double durationMs() const {
        return (endTimeUs - startTimeUs) / 1000.0;
    }

    /**
     * Get the top N hottest functions by self time.
     */
    std::vector<const ProfileNode*> hotFunctions(size_t n) const {
        std::vector<const ProfileNode*> sorted;
        for (const auto& node : nodes) {
            if (node.selfSamples > 0) sorted.push_back(&node);
        }
        std::sort(sorted.begin(), sorted.end(),
            [](const ProfileNode* a, const ProfileNode* b) {
                return a->selfSamples > b->selfSamples;
            });
        if (sorted.size() > n) sorted.resize(n);
        return sorted;
    }

    void dump() const {
        printf("CPU Profile: %.1fms, %u samples\n", durationMs(), totalSamples);
        auto hot = hotFunctions(10);
        for (const auto* node : hot) {
            printf("  %6.1fms %6.1fms  %s (%s:%u)\n",
                   node->selfTimeMs(samplingIntervalUs),
                   node->totalTimeMs(samplingIntervalUs),
                   node->frame.functionName.c_str(),
                   node->frame.scriptUrl.c_str(),
                   node->frame.lineNumber);
        }
    }
};

// =============================================================================
// CPU Profiler
// =============================================================================

class CpuProfiler {
public:
    using StackSampler = std::function<std::vector<ProfileFrame>()>;

    CpuProfiler() : running_(false), samplingIntervalUs_(1000), nextNodeId_(0) {}

    ~CpuProfiler() { stop(); }

    // =========================================================================
    // Configuration
    // =========================================================================

    void setSamplingInterval(uint32_t microseconds) {
        samplingIntervalUs_ = microseconds;
    }

    /**
     * Set the callback that returns the current call stack.
     * Must be thread-safe (called from sampling thread).
     */
    void setStackSampler(StackSampler sampler) {
        sampler_ = std::move(sampler);
    }

    // =========================================================================
    // Control
    // =========================================================================

    void start() {
        if (running_.load()) return;
        running_ = true;
        samples_.clear();
        nodeMap_.clear();
        nodes_.clear();
        nextNodeId_ = 0;

        auto now = std::chrono::steady_clock::now();
        startTime_ = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();

        // Create root node
        ProfileFrame rootFrame;
        rootFrame.functionName = "(root)";
        getOrCreateNode(rootFrame);

        samplingThread_ = std::thread(&CpuProfiler::samplingLoop, this);
    }

    ProfileResult stop() {
        running_ = false;
        if (samplingThread_.joinable()) {
            samplingThread_.join();
        }

        auto now = std::chrono::steady_clock::now();
        uint64_t endTime = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();

        // Build result from samples
        ProfileResult result;
        result.startTimeUs = startTime_;
        result.endTimeUs = endTime;
        result.totalSamples = static_cast<uint32_t>(samples_.size());
        result.samplingIntervalUs = samplingIntervalUs_;
        result.nodes = nodes_;

        return result;
    }

    bool isRunning() const { return running_.load(); }

private:
    void samplingLoop() {
        auto interval = std::chrono::microseconds(samplingIntervalUs_);

        while (running_.load()) {
            auto start = std::chrono::steady_clock::now();

            takeSample();

            auto elapsed = std::chrono::steady_clock::now() - start;
            auto remaining = interval - elapsed;
            if (remaining.count() > 0) {
                std::this_thread::sleep_for(remaining);
            }
        }
    }

    void takeSample() {
        if (!sampler_) return;

        auto stack = sampler_();
        if (stack.empty()) return;

        std::lock_guard<std::mutex> lock(mutex_);
        samples_.push_back(stack);

        // Update profile tree
        uint32_t parentId = 0; // Start at root

        for (size_t i = stack.size(); i > 0; --i) {
            const auto& frame = stack[i - 1];
            uint32_t nodeId = getOrCreateNode(frame);

            // Link parent → child if not already linked
            auto& parent = nodes_[parentId];
            auto it = std::find(parent.children.begin(),
                                parent.children.end(), nodeId);
            if (it == parent.children.end()) {
                parent.children.push_back(nodeId);
            }

            nodes_[nodeId].totalSamples++;
            parentId = nodeId;
        }

        // Top of stack gets self sample
        if (!stack.empty()) {
            uint32_t topId = getOrCreateNode(stack.front());
            nodes_[topId].selfSamples++;
        }
    }

    uint32_t getOrCreateNode(const ProfileFrame& frame) {
        auto it = nodeMap_.find(frame);
        if (it != nodeMap_.end()) return it->second;

        uint32_t id = nextNodeId_++;
        ProfileNode node;
        node.id = id;
        node.frame = frame;
        nodes_.push_back(node);
        nodeMap_[frame] = id;
        return id;
    }

    std::atomic<bool> running_;
    uint32_t samplingIntervalUs_;
    uint64_t startTime_ = 0;
    uint32_t nextNodeId_;

    StackSampler sampler_;
    std::thread samplingThread_;
    std::mutex mutex_;

    std::vector<std::vector<ProfileFrame>> samples_;
    std::vector<ProfileNode> nodes_;
    std::unordered_map<ProfileFrame, uint32_t, ProfileFrameHash> nodeMap_;
};



} // namespace Zepra::Profiler
