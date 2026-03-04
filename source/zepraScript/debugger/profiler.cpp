/**
 * @file profiler.cpp
 * @brief CPU and Memory profiler implementation
 */

#include "debugger/profiler.hpp"
#include <algorithm>

namespace Zepra::Debug {

// =============================================================================
// CPUProfiler
// =============================================================================

void CPUProfiler::start(const std::string& name) {
    if (running_) return;
    
    running_ = true;
    profileName_ = name;
    startTime_ = std::chrono::steady_clock::now();
    nodes_.clear();
    samples_.clear();
    callStack_.clear();
    nodeMap_.clear();
    nextNodeId_ = 0;
    
    // Create root node
    ProfileNode root;
    root.id = nextNodeId_++;
    root.functionName = "(root)";
    nodes_.push_back(root);
    callStack_.push_back(0);
}

std::unique_ptr<CPUProfile> CPUProfiler::stop() {
    if (!running_) return nullptr;
    
    running_ = false;
    auto endTime = std::chrono::steady_clock::now();
    
    auto profile = std::make_unique<CPUProfile>();
    profile->name = profileName_;
    profile->startTime = 0;
    profile->endTime = std::chrono::duration<double, std::milli>(
        endTime - startTime_).count();
    profile->nodes = std::move(nodes_);
    profile->samples = std::move(samples_);
    
    return profile;
}

void CPUProfiler::onFunctionEnter(const std::string& functionName, 
                                   const std::string& file, int line) {
    if (!running_) return;
    
    int nodeId = getOrCreateNode(functionName, file, line);
    
    // Add as child of current node
    if (!callStack_.empty()) {
        int parentId = callStack_.back();
        auto& children = nodes_[parentId].children;
        if (std::find(children.begin(), children.end(), nodeId) == children.end()) {
            children.push_back(nodeId);
        }
        nodes_[nodeId].parent = parentId;
    }
    
    callStack_.push_back(nodeId);
    nodes_[nodeId].hitCount++;
    
    takeSample();
}

void CPUProfiler::onFunctionExit() {
    if (!running_ || callStack_.size() <= 1) return;
    callStack_.pop_back();
    takeSample();
}

void CPUProfiler::takeSample() {
    if (callStack_.empty()) return;
    
    auto now = std::chrono::steady_clock::now();
    ProfileSample sample;
    sample.nodeId = callStack_.back();
    sample.timestamp = std::chrono::duration<double, std::milli>(
        now - startTime_).count();
    samples_.push_back(sample);
}

int CPUProfiler::getOrCreateNode(const std::string& name, 
                                  const std::string& file, int line) {
    std::string key = name + ":" + file + ":" + std::to_string(line);
    
    auto it = nodeMap_.find(key);
    if (it != nodeMap_.end()) {
        return it->second;
    }
    
    ProfileNode node;
    node.id = nextNodeId_++;
    node.functionName = name;
    node.sourceFile = file;
    node.lineNumber = line;
    nodes_.push_back(node);
    
    nodeMap_[key] = node.id;
    return node.id;
}

// =============================================================================
// MemoryProfiler
// =============================================================================

std::unique_ptr<HeapSnapshot> MemoryProfiler::takeSnapshot(const std::string& name) {
    auto snapshot = std::make_unique<HeapSnapshot>();
    snapshot->name = name;
    snapshot->timestamp = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Walk heap and collect objects
    snapshot->totalSize = 0;
    snapshot->totalObjects = 0;
    
    for (const auto& [ptr, info] : allocations_) {
        HeapNode node;
        node.id = static_cast<int>(snapshot->nodes.size());
        node.type = info.second;
        node.selfSize = info.first;
        node.retainedSize = info.first;  // Simplified
        snapshot->nodes.push_back(node);
        
        snapshot->totalSize += info.first;
        snapshot->totalObjects++;
    }
    
    return snapshot;
}

void MemoryProfiler::startTrackingAllocations() {
    trackingAllocations_ = true;
    trackedAllocations_.clear();
}

std::vector<std::pair<std::string, size_t>> MemoryProfiler::stopTrackingAllocations() {
    trackingAllocations_ = false;
    return std::move(trackedAllocations_);
}

size_t MemoryProfiler::getUsedHeapSize() const {
    size_t total = 0;
    for (const auto& [ptr, info] : allocations_) {
        total += info.first;
    }
    return total;
}

size_t MemoryProfiler::getTotalHeapSize() const {
    // TODO: Get from GC heap
    return getUsedHeapSize() * 2;  // Estimate
}

void MemoryProfiler::onAllocation(void* ptr, size_t size, const std::string& type) {
    allocations_[ptr] = {size, type};
    
    if (trackingAllocations_) {
        trackedAllocations_.emplace_back(type, size);
    }
}

void MemoryProfiler::onDeallocation(void* ptr) {
    allocations_.erase(ptr);
}

// =============================================================================
// Timeline
// =============================================================================

void Timeline::start() {
    recording_ = true;
    events_.clear();
}

void Timeline::stop() {
    recording_ = false;
}

void Timeline::addEvent(const TimelineEvent& event) {
    if (recording_) {
        events_.push_back(event);
    }
}

} // namespace Zepra::Debug
