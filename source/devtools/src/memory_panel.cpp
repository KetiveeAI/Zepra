/**
 * @file memory_panel.cpp
 * @brief Memory/heap profiler panel implementation
 */

#include "devtools/memory_panel.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Zepra::DevTools {

MemoryPanel::MemoryPanel() : DevToolsPanel("Memory") {}

MemoryPanel::~MemoryPanel() = default;

void MemoryPanel::render() {
    // Render heap usage, allocation timeline, and snapshot comparison
}

void MemoryPanel::refresh() {
    if (onRefresh_) {
        onRefresh_();
    }
}

void MemoryPanel::takeSnapshot() {
    HeapSnapshot snapshot;
    snapshot.id = nextSnapshotId_++;
    snapshot.timestamp = std::chrono::system_clock::now();
    
    // Get current heap stats
    if (onTakeSnapshot_) {
        snapshot = onTakeSnapshot_();
        snapshot.id = nextSnapshotId_ - 1;  // Restore our ID
    }
    
    snapshots_.push_back(snapshot);
    
    if (onSnapshotTaken_) {
        onSnapshotTaken_(snapshot);
    }
}

void MemoryPanel::deleteSnapshot(int snapshotId) {
    snapshots_.erase(
        std::remove_if(snapshots_.begin(), snapshots_.end(),
            [snapshotId](const HeapSnapshot& s) { return s.id == snapshotId; }),
        snapshots_.end());
}

std::vector<HeapSnapshot> MemoryPanel::getSnapshots() {
    return snapshots_;
}

const HeapSnapshot* MemoryPanel::getSnapshot(int snapshotId) {
    for (const auto& snapshot : snapshots_) {
        if (snapshot.id == snapshotId) {
            return &snapshot;
        }
    }
    return nullptr;
}

SnapshotDiff MemoryPanel::compareSnapshots(int snapshot1Id, int snapshot2Id) {
    SnapshotDiff diff;
    
    const HeapSnapshot* s1 = getSnapshot(snapshot1Id);
    const HeapSnapshot* s2 = getSnapshot(snapshot2Id);
    
    if (!s1 || !s2) {
        return diff;
    }
    
    diff.snapshot1Id = snapshot1Id;
    diff.snapshot2Id = snapshot2Id;
    diff.sizeDelta = static_cast<int64_t>(s2->totalSize) - 
                     static_cast<int64_t>(s1->totalSize);
    diff.countDelta = static_cast<int64_t>(s2->objectCount) - 
                      static_cast<int64_t>(s1->objectCount);
    
    // Build type diffs
    std::unordered_map<std::string, int64_t> typeCounts1, typeCounts2;
    std::unordered_map<std::string, int64_t> typeSizes1, typeSizes2;
    
    for (const auto& obj : s1->objects) {
        typeCounts1[obj.type]++;
        typeSizes1[obj.type] += obj.size;
    }
    
    for (const auto& obj : s2->objects) {
        typeCounts2[obj.type]++;
        typeSizes2[obj.type] += obj.size;
    }
    
    // Find added and removed
    std::set<std::string> allTypes;
    for (const auto& [type, _] : typeCounts1) allTypes.insert(type);
    for (const auto& [type, _] : typeCounts2) allTypes.insert(type);
    
    for (const auto& type : allTypes) {
        TypeDiff td;
        td.typeName = type;
        td.countDelta = typeCounts2[type] - typeCounts1[type];
        td.sizeDelta = typeSizes2[type] - typeSizes1[type];
        
        if (td.countDelta != 0 || td.sizeDelta != 0) {
            diff.byType.push_back(td);
        }
    }
    
    // Sort by size delta (absolute)
    std::sort(diff.byType.begin(), diff.byType.end(),
        [](const TypeDiff& a, const TypeDiff& b) {
            return std::abs(a.sizeDelta) > std::abs(b.sizeDelta);
        });
    
    return diff;
}

MemoryStats MemoryPanel::getCurrentStats() {
    MemoryStats stats;
    
    if (onGetStats_) {
        stats = onGetStats_();
    }
    
    return stats;
}

void MemoryPanel::startAllocationTracking() {
    isTrackingAllocations_ = true;
    allocations_.clear();
    
    if (onStartTracking_) {
        onStartTracking_();
    }
}

void MemoryPanel::stopAllocationTracking() {
    isTrackingAllocations_ = false;
    
    if (onStopTracking_) {
        onStopTracking_();
    }
}

void MemoryPanel::recordAllocation(const AllocationRecord& record) {
    if (isTrackingAllocations_) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.push_back(record);
    }
}

std::vector<AllocationRecord> MemoryPanel::getAllocations() {
    std::lock_guard<std::mutex> lock(mutex_);
    return allocations_;
}

void MemoryPanel::forceGC() {
    if (onForceGC_) {
        onForceGC_();
    }
}

std::string MemoryPanel::formatSize(size_t bytes) {
    std::ostringstream oss;
    
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
    } else {
        oss << std::fixed << std::setprecision(2) 
            << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    }
    
    return oss.str();
}

} // namespace Zepra::DevTools
