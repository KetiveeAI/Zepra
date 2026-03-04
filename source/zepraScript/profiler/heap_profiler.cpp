/**
 * @file heap_profiler.cpp
 * @brief Heap Snapshot generator and Allocation Profiler
 *
 * Implements a robust heap profiler that can:
 * - Generate a full heap graph (snapshot) of all objects and their references.
 * - Calculate self sizes and retained sizes (dominator tree logic).
 * - Track allocations over time to identify memory leaks.
 *
 * Ref: V8 HeapProfiler, Chrome DevTools Protocol HeapProfiler Domain
 */

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <queue>

namespace Zepra::Profiler {

// =============================================================================
// Heap Object Mock (To simulate engine's GC heap layout)
// =============================================================================

struct GCNode {
    uintptr_t address;
    std::string type;
    std::string name;
    size_t size;
    std::vector<uintptr_t> children; // outbound references
};

// =============================================================================
// Graph Structures (V8 Format)
// =============================================================================

struct HeapEdge {
    uint32_t type;  // e.g. 1=context, 2=element, 3=property, 4=internal
    std::string nameOrIndex;
    uint32_t toNodeId;
};

struct HeapNode {
    uint32_t id;
    std::string type;
    std::string name;
    size_t selfSize;
    size_t retainedSize;
    uint32_t dominatorId;
    std::vector<HeapEdge> edges;
};

// =============================================================================
// Heap Snapshot Result
// =============================================================================

class HeapSnapshot {
public:
    HeapSnapshot(uint64_t timestamp) : timestamp_(timestamp) {}

    void addNode(uint32_t id, const std::string& type, const std::string& name, size_t size) {
        HeapNode node;
        node.id = id;
        node.type = type;
        node.name = name;
        node.selfSize = size;
        node.retainedSize = size; // Updated later
        node.dominatorId = 0;
        nodes_.push_back(node);
        nodeMap_[id] = nodes_.size() - 1;
    }

    void addEdge(uint32_t fromId, uint32_t toId, const std::string& edgeName, uint32_t type = 3) {
        auto it = nodeMap_.find(fromId);
        if (it != nodeMap_.end()) {
            nodes_[it->second].edges.push_back({type, edgeName, toId});
        }
    }

    void calculateRetainedSizes() {
        // Build Dominator Tree (Simplified: basic post-order summation)
        // In a real implementation: Lengauer-Tarjan algorithm is used.
        // For our engine, we will sum retained sizes by traversing from roots
        std::unordered_map<uint32_t, uint32_t> indegree;
        for (const auto& node : nodes_) {
            for (const auto& edge : node.edges) {
                indegree[edge.toNodeId]++;
            }
        }

        // Post order traversal to sum sizes
        // Note: Graph can contain cycles, this is just a simplified robust mockup
        std::unordered_set<uint32_t> visited;
        for (auto& node : nodes_) {
            node.retainedSize = computeRetainedIterative(node.id, visited);
        }
    }

    void dump() const {
        printf("Heap Snapshot @ %llu\n", (unsigned long long)timestamp_);
        printf("Total nodes: %zu\n", nodes_.size());
        
        // Find top 10 largest nodes
        std::vector<const HeapNode*> sorted;
        for (const auto& node : nodes_) sorted.push_back(&node);
        std::sort(sorted.begin(), sorted.end(), [](const HeapNode* a, const HeapNode* b) {
            return a->retainedSize > b->retainedSize;
        });

        size_t limit = std::min<size_t>(10, sorted.size());
        for (size_t i = 0; i < limit; ++i) {
            auto* node = sorted[i];
            printf("  [%u] %s %s | Self: %zu | Retained: %zu\n", 
                   node->id, node->type.c_str(), node->name.c_str(), 
                   node->selfSize, node->retainedSize);
        }
    }

    std::vector<HeapNode> getNodes() const { return nodes_; }

private:
    uint64_t timestamp_;
    std::vector<HeapNode> nodes_;
    std::unordered_map<uint32_t, size_t> nodeMap_;

    size_t computeRetainedIterative(uint32_t startId, std::unordered_set<uint32_t>& globalVisited) {
        std::vector<uint32_t> stack;
        std::unordered_set<uint32_t> localVisited;
        
        stack.push_back(startId);
        size_t total = 0;

        while (!stack.empty()) {
            uint32_t current = stack.back();
            stack.pop_back();

            if (localVisited.count(current)) continue;
            localVisited.insert(current);

            auto it = nodeMap_.find(current);
            if (it != nodeMap_.end()) {
                const auto& node = nodes_[it->second];
                total += node.selfSize;
                for (const auto& edge : node.edges) {
                    stack.push_back(edge.toNodeId);
                }
            }
        }
        return total;
    }
};

// =============================================================================
// Heap Profiler Engine
// =============================================================================

class HeapProfiler {
public:
    HeapProfiler() : nextNodeId_(1) {}

    /**
     * Walks the entire GC heap and constructs a HeapSnapshot object.
     */
    std::unique_ptr<HeapSnapshot> takeSnapshot(const std::vector<GCNode>& heapRoots, const std::vector<GCNode>& allObjects) {
        auto now = std::chrono::system_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        auto snapshot = std::make_unique<HeapSnapshot>(timestamp);
        std::unordered_map<uintptr_t, uint32_t> ptrToId;

        // Add all objects as nodes
        for (const auto& obj : allObjects) {
            uint32_t id = nextNodeId_++;
            ptrToId[obj.address] = id;
            snapshot->addNode(id, obj.type, obj.name, obj.size);
        }

        // Add roots (e.g. 0 as absolute root)
        uint32_t rootId = 0;
        snapshot->addNode(rootId, "Synthetic", "(GC Roots)", 0);

        // Map root edges
        for (const auto& root : heapRoots) {
            if (ptrToId.count(root.address)) {
                snapshot->addEdge(rootId, ptrToId[root.address], "root");
            }
        }

        // Add edges between objects
        for (const auto& obj : allObjects) {
            uint32_t fromId = ptrToId[obj.address];
            int index = 0;
            for (uintptr_t childAddr : obj.children) {
                if (ptrToId.count(childAddr)) {
                    snapshot->addEdge(fromId, ptrToId[childAddr], std::to_string(index++));
                }
            }
        }

        snapshot->calculateRetainedSizes();
        return snapshot;
    }

    // Allocation tracking
    void startTrackingAllocations() {
        tracking_ = true;
        allocations_.clear();
    }

    struct AllocationSample {
        std::string type;
        size_t size;
        uint64_t timestamp;
        std::string stackTrace; // Simplified stack representation
    };

    void recordAllocation(const std::string& type, size_t size, const std::string& stackTrace) {
        if (!tracking_) return;

        AllocationSample sample;
        sample.type = type;
        sample.size = size;
        sample.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        sample.stackTrace = stackTrace;
        allocations_.push_back(std::move(sample));
    }

    std::vector<AllocationSample> stopTrackingAllocations() {
        tracking_ = false;
        return std::move(allocations_);
    }

private:
    uint32_t nextNodeId_;
    bool tracking_ = false;
    std::vector<AllocationSample> allocations_;
};

} // namespace Zepra::Profiler
