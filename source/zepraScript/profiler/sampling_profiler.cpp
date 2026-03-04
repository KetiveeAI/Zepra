/**
 * @file sampling_profiler.cpp
 * @brief Statistical Allocation Sampling Profiler
 *
 * Implements a low-overhead memory allocation profiler that samples
 * allocations statistically (using Poisson process intervals) instead
 * of tracking every single allocation.
 *
 * This allows safe profiling of production workloads to find memory hotspots.
 *
 * Ref: V8 SamplingHeapProfiler, tcmalloc allocation sampling
 */

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <algorithm>

namespace Zepra::Profiler {

// We borrow ProfileFrame from cpu_profiler, mock it here for independence
struct AllocProfileFrame {
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber;
    uint32_t columnNumber;

    bool operator==(const AllocProfileFrame& other) const {
        return functionName == other.functionName &&
               scriptUrl == other.scriptUrl &&
               lineNumber == other.lineNumber;
    }
};

struct AllocProfileFrameHash {
    size_t operator()(const AllocProfileFrame& f) const {
        size_t h = std::hash<std::string>{}(f.functionName);
        h ^= std::hash<std::string>{}(f.scriptUrl) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(f.lineNumber) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// =============================================================================
// Sampling Profile Node (Call Tree)
// =============================================================================

struct SamplingAllocationNode {
    uint32_t id;
    AllocProfileFrame frame;
    size_t selfAllocatedBytes = 0;
    size_t totalAllocatedBytes = 0;
    size_t allocationCount = 0;
    std::vector<uint32_t> children;
};

// =============================================================================
// Profiler Profile Result
// =============================================================================

struct SamplingAllocationProfile {
    std::vector<SamplingAllocationNode> nodes;
    size_t totalAllocatedBytes = 0;
    size_t totalSamples = 0;

    void dump() const {
        printf("Sampling Allocation Profile: %zu bytes, %zu samples\n", 
               totalAllocatedBytes, totalSamples);
               
        // Find top 10 allocators
        std::vector<const SamplingAllocationNode*> sorted;
        for (const auto& node : nodes) {
            if (node.selfAllocatedBytes > 0) {
                sorted.push_back(&node);
            }
        }
        std::sort(sorted.begin(), sorted.end(), 
            [](const SamplingAllocationNode* a, const SamplingAllocationNode* b) {
                return a->selfAllocatedBytes > b->selfAllocatedBytes;
            });

        size_t limit = std::min<size_t>(10, sorted.size());
        for (size_t i = 0; i < limit; ++i) {
            const auto* node = sorted[i];
            printf("  %8zu bytes (%4zu samples)  %s (%s:%u)\n",
                   node->selfAllocatedBytes,
                   node->allocationCount,
                   node->frame.functionName.c_str(),
                   node->frame.scriptUrl.c_str(),
                   node->frame.lineNumber);
        }
    }
};

// =============================================================================
// Sampling Heap Profiler
// =============================================================================

class SamplingHeapProfiler {
public:
    SamplingHeapProfiler(size_t sampleIntervalBytes = 512 * 1024)
        : sampleIntervalBytes_(sampleIntervalBytes),
          running_(false),
          bytesAllocatedSinceLastSample_(0),
          nextNodeId_(0) 
    {
        updateNextSampleInterval();
    }

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) return;

        nodes_.clear();
        nodeMap_.clear();
        nextNodeId_ = 0;
        
        // Create root node
        AllocProfileFrame root{"(root)", "", 0, 0};
        getOrCreateNode(root);

        bytesAllocatedSinceLastSample_ = 0;
        running_ = true;
        updateNextSampleInterval();
    }

    SamplingAllocationProfile stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;

        SamplingAllocationProfile profile;
        profile.nodes = nodes_;
        profile.totalAllocatedBytes = 0;
        profile.totalSamples = 0;

        for (const auto& node : nodes_) {
            profile.totalAllocatedBytes += node.selfAllocatedBytes;
            profile.totalSamples += node.allocationCount;
        }

        return profile;
    }

    /**
     * Called on EVERY allocation (e.g. from the GC heap).
     * If the interval is reached, a sample is taken.
     */
    void onAllocate(size_t bytes, const std::vector<AllocProfileFrame>& currentStack) {
        if (!running_) return;

        bytesAllocatedSinceLastSample_ += bytes;
        if (bytesAllocatedSinceLastSample_ >= nextSampleIntervalBytes_) {
            // Trigger sample
            std::lock_guard<std::mutex> lock(mutex_);
            recordSample(currentStack, bytesAllocatedSinceLastSample_);
            
            bytesAllocatedSinceLastSample_ = 0;
            updateNextSampleInterval();
        }
    }

private:
    void updateNextSampleInterval() {
        // Use Poisson distribution to randomize the sample interval
        // This avoids resonance with regular allocation patterns
        static thread_local std::mt19937 generator(std::random_device{}());
        std::exponential_distribution<double> distribution(1.0 / sampleIntervalBytes_);
        
        nextSampleIntervalBytes_ = std::max<size_t>(1, static_cast<size_t>(distribution(generator)));
    }

    void recordSample(const std::vector<AllocProfileFrame>& stack, size_t bytesRepresented) {
        if (stack.empty()) return;

        uint32_t parentId = 0; // Root is always 0
        nodes_[0].totalAllocatedBytes += bytesRepresented;

        // Traverse stack top-down to build tree
        for (size_t i = stack.size(); i > 0; --i) {
            const auto& frame = stack[i - 1];
            uint32_t nodeId = getOrCreateNode(frame);

            // Add edge from parent to child
            auto& parentNode = nodes_[parentId];
            auto it = std::find(parentNode.children.begin(), parentNode.children.end(), nodeId);
            if (it == parentNode.children.end()) {
                parentNode.children.push_back(nodeId);
            }

            nodes_[nodeId].totalAllocatedBytes += bytesRepresented;
            parentId = nodeId;
        }

        // The exact allocating frame gets the self bytes
        uint32_t topNodeId = parentId;
        nodes_[topNodeId].selfAllocatedBytes += bytesRepresented;
        nodes_[topNodeId].allocationCount++;
    }

    uint32_t getOrCreateNode(const AllocProfileFrame& frame) {
        auto it = nodeMap_.find(frame);
        if (it != nodeMap_.end()) {
            return it->second;
        }

        uint32_t id = nextNodeId_++;
        SamplingAllocationNode node;
        node.id = id;
        node.frame = frame;
        nodes_.push_back(std::move(node));
        nodeMap_[frame] = id;
        return id;
    }

    size_t sampleIntervalBytes_;
    size_t nextSampleIntervalBytes_;
    bool running_;
    size_t bytesAllocatedSinceLastSample_;
    uint32_t nextNodeId_;
    std::mutex mutex_;
    std::vector<SamplingAllocationNode> nodes_;
    std::unordered_map<AllocProfileFrame, uint32_t, AllocProfileFrameHash> nodeMap_;
};

} // namespace Zepra::Profiler
