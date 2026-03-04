/**
 * @file HeapProfiler.h
 * @brief Heap Snapshot and Allocation Tracking
 * 
 * V8/JSC HeapProfiler patterns:
 * - Heap snapshots with object graph
 * - Allocation tracking by site
 * - Retained size calculation
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Zepra::Perf {

// =============================================================================
// Heap Object Info
// =============================================================================

/**
 * @brief Information about a heap object
 */
struct HeapObjectInfo {
    uintptr_t address;
    size_t shallowSize;       // Object's own size
    size_t retainedSize = 0;  // Size including children
    
    std::string typeName;
    std::string constructorName;
    
    // For objects with names
    std::string name;
    
    // References
    std::vector<uintptr_t> outgoingEdges;
    std::vector<uintptr_t> incomingEdges;
    
    // Allocation info
    uint32_t allocationSiteId = 0;
    std::string allocationStack;
};

// =============================================================================
// Heap Edge
// =============================================================================

enum class HeapEdgeType : uint8_t {
    Context,    // Variable in closure
    Element,    // Array element
    Property,   // Named property
    Internal,   // Internal reference
    Hidden,     // Hidden reference
    Shortcut,   // Shortcut (not counted in size)
    Weak        // Weak reference
};

/**
 * @brief Edge in object graph
 */
struct HeapEdge {
    HeapEdgeType type;
    std::string name;       // Property name or index
    uintptr_t toNode;
};

// =============================================================================
// Allocation Site
// =============================================================================

/**
 * @brief Tracks allocations from a specific code location
 */
struct AllocationSite {
    uint32_t id;
    std::string functionName;
    std::string sourceFile;
    int lineNumber;
    int columnNumber;
    
    // Statistics
    uint64_t totalAllocations = 0;
    uint64_t totalBytes = 0;
    uint64_t liveCount = 0;
    size_t liveBytes = 0;
};

// =============================================================================
// Heap Snapshot
// =============================================================================

/**
 * @brief Complete heap snapshot
 */
class HeapSnapshot {
public:
    HeapSnapshot() = default;
    
    // Add object
    void AddObject(HeapObjectInfo info) {
        objects_[info.address] = std::move(info);
        objectCount_++;
    }
    
    // Add edge
    void AddEdge(uintptr_t from, HeapEdge edge) {
        edges_[from].push_back(std::move(edge));
    }
    
    // Get object
    const HeapObjectInfo* GetObject(uintptr_t addr) const {
        auto it = objects_.find(addr);
        return it != objects_.end() ? &it->second : nullptr;
    }
    
    // Statistics
    size_t ObjectCount() const { return objectCount_; }
    size_t TotalSize() const { return totalSize_; }
    
    // Calculate retained sizes (dominator tree)
    void CalculateRetainedSizes() {
        // Build dominator tree
        ComputeDominators();
        
        // Calculate retained sizes bottom-up
        for (auto& [addr, obj] : objects_) {
            obj.retainedSize = CalculateRetainedSize(addr);
        }
    }
    
    // Find objects by type
    std::vector<const HeapObjectInfo*> FindByType(const std::string& typeName) const {
        std::vector<const HeapObjectInfo*> result;
        for (const auto& [_, obj] : objects_) {
            if (obj.typeName == typeName) {
                result.push_back(&obj);
            }
        }
        return result;
    }
    
    // Get roots
    std::vector<uintptr_t> GetRoots() const { return roots_; }
    void AddRoot(uintptr_t addr) { roots_.push_back(addr); }
    
    // Serialize to JSON (for DevTools)
    std::string ToJSON() const;
    
private:
    void ComputeDominators() {
        // Would implement dominator tree algorithm
    }
    
    size_t CalculateRetainedSize(uintptr_t addr) {
        auto it = objects_.find(addr);
        if (it == objects_.end()) return 0;
        
        size_t size = it->second.shallowSize;
        
        for (const auto& edge : edges_[addr]) {
            if (edge.type != HeapEdgeType::Weak) {
                size += CalculateRetainedSize(edge.toNode);
            }
        }
        
        return size;
    }
    
    std::unordered_map<uintptr_t, HeapObjectInfo> objects_;
    std::unordered_map<uintptr_t, std::vector<HeapEdge>> edges_;
    std::vector<uintptr_t> roots_;
    size_t objectCount_ = 0;
    size_t totalSize_ = 0;
};

// =============================================================================
// Allocation Tracker
// =============================================================================

/**
 * @brief Tracks allocation sites for memory profiling
 */
class AllocationTracker {
public:
    static AllocationTracker& Instance() {
        static AllocationTracker tracker;
        return tracker;
    }
    
    // Start/stop tracking
    void Start() { enabled_ = true; }
    void Stop() { enabled_ = false; }
    bool IsEnabled() const { return enabled_; }
    
    // Record allocation
    uint32_t RecordAllocation(size_t size, 
                               const std::string& functionName,
                               const std::string& sourceFile,
                               int line, int column) {
        if (!enabled_) return 0;
        
        std::string key = sourceFile + ":" + std::to_string(line) + 
                          ":" + std::to_string(column);
        
        auto it = sitesByLocation_.find(key);
        if (it != sitesByLocation_.end()) {
            AllocationSite* site = sites_[it->second].get();
            site->totalAllocations++;
            site->totalBytes += size;
            site->liveCount++;
            site->liveBytes += size;
            return site->id;
        }
        
        // New site
        uint32_t id = nextSiteId_++;
        auto site = std::make_unique<AllocationSite>();
        site->id = id;
        site->functionName = functionName;
        site->sourceFile = sourceFile;
        site->lineNumber = line;
        site->columnNumber = column;
        site->totalAllocations = 1;
        site->totalBytes = size;
        site->liveCount = 1;
        site->liveBytes = size;
        
        sitesByLocation_[key] = id;
        sites_[id] = std::move(site);
        
        return id;
    }
    
    // Record deallocation
    void RecordDeallocation(uint32_t siteId, size_t size) {
        auto it = sites_.find(siteId);
        if (it != sites_.end()) {
            if (it->second->liveCount > 0) {
                it->second->liveCount--;
                it->second->liveBytes -= size;
            }
        }
    }
    
    // Get top allocation sites
    std::vector<const AllocationSite*> GetTopSites(size_t limit = 10) const {
        std::vector<const AllocationSite*> sorted;
        for (const auto& [_, site] : sites_) {
            sorted.push_back(site.get());
        }
        
        std::sort(sorted.begin(), sorted.end(),
            [](const AllocationSite* a, const AllocationSite* b) {
                return a->liveBytes > b->liveBytes;
            });
        
        if (sorted.size() > limit) {
            sorted.resize(limit);
        }
        return sorted;
    }
    
    void Clear() {
        sites_.clear();
        sitesByLocation_.clear();
        nextSiteId_ = 1;
    }
    
private:
    AllocationTracker() = default;
    
    bool enabled_ = false;
    uint32_t nextSiteId_ = 1;
    std::unordered_map<uint32_t, std::unique_ptr<AllocationSite>> sites_;
    std::unordered_map<std::string, uint32_t> sitesByLocation_;
};

// =============================================================================
// Heap Profiler
// =============================================================================

/**
 * @brief Main heap profiler interface
 */
class HeapProfiler {
public:
    // Take heap snapshot
    std::unique_ptr<HeapSnapshot> TakeSnapshot() {
        auto snapshot = std::make_unique<HeapSnapshot>();
        
        // Would walk heap and populate snapshot
        
        snapshot->CalculateRetainedSizes();
        return snapshot;
    }
    
    // Start allocation tracking
    void StartAllocationTracking() {
        AllocationTracker::Instance().Start();
    }
    
    // Stop allocation tracking
    void StopAllocationTracking() {
        AllocationTracker::Instance().Stop();
    }
    
    // Get allocation profile
    std::vector<const AllocationSite*> GetAllocationProfile() const {
        return AllocationTracker::Instance().GetTopSites(50);
    }
};

} // namespace Zepra::Perf
