/**
 * @file memory_panel.hpp
 * @brief Memory Panel - Heap snapshots, allocation tracking
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>

namespace Zepra::DevTools {

/**
 * @brief Object type in heap
 */
enum class HeapObjectType {
    Object,
    Array,
    String,
    Function,
    Closure,
    RegExp,
    Date,
    Map,
    Set,
    ArrayBuffer,
    TypedArray,
    Native,
    Hidden,
    Code,
    Synthetic
};

/**
 * @brief Heap object info
 */
struct HeapObject {
    uint64_t id;
    HeapObjectType type;
    std::string className;
    std::string name;
    
    size_t shallowSize;     // Size of object itself
    size_t retainedSize;    // Size + all referenced objects
    
    int distance;           // Distance from root
    uint64_t retainerId;    // Object that retains this
    
    // References
    std::vector<uint64_t> references;
    std::vector<uint64_t> referrers;
};

/**
 * @brief Heap snapshot
 */
struct HeapSnapshot {
    int id;
    std::string title;
    std::chrono::system_clock::time_point timestamp;
    
    size_t totalSize;
    size_t usedSize;
    int objectCount;
    int rootCount;
    
    std::vector<HeapObject> objects;
    
    // Statistics by type
    std::vector<std::pair<std::string, size_t>> byClass;
    std::vector<std::pair<HeapObjectType, size_t>> byType;
};

/**
 * @brief Allocation record
 */
struct AllocationRecord {
    uint64_t objectId;
    std::string className;
    size_t size;
    double timestamp;
    
    std::string function;
    std::string url;
    int line;
};

/**
 * @brief Memory comparison result
 */
struct SnapshotDiff {
    int snapshotId1;
    int snapshotId2;
    
    struct ClassDiff {
        std::string className;
        int addedCount;
        int removedCount;
        int delta;
        size_t addedSize;
        size_t removedSize;
        int64_t sizeDelta;
    };
    
    std::vector<ClassDiff> byClass;
    size_t totalAdded;
    size_t totalRemoved;
    int64_t sizeDelta;
};

/**
 * @brief Memory callbacks
 */
using SnapshotCallback = std::function<void(const HeapSnapshot&)>;
using AllocationCallback = std::function<void(const AllocationRecord&)>;

/**
 * @brief Memory Panel - Heap Profiler
 */
class MemoryPanel {
public:
    MemoryPanel();
    ~MemoryPanel();
    
    // --- Snapshots ---
    
    /**
     * @brief Take heap snapshot
     */
    HeapSnapshot takeSnapshot(const std::string& title = "");
    
    /**
     * @brief Get all snapshots
     */
    const std::vector<HeapSnapshot>& snapshots() const { return snapshots_; }
    
    /**
     * @brief Select snapshot
     */
    void selectSnapshot(int id);
    int selectedSnapshot() const { return selectedSnapshotId_; }
    
    /**
     * @brief Delete snapshot
     */
    void deleteSnapshot(int id);
    
    /**
     * @brief Clear all snapshots
     */
    void clearSnapshots();
    
    // --- Comparison ---
    
    /**
     * @brief Compare two snapshots
     */
    SnapshotDiff compareSnapshots(int id1, int id2);
    
    // --- Allocation Tracking ---
    
    /**
     * @brief Start allocation tracking
     */
    void startTracking();
    
    /**
     * @brief Stop allocation tracking
     */
    void stopTracking();
    
    /**
     * @brief Is tracking
     */
    bool isTracking() const { return tracking_; }
    
    /**
     * @brief Get allocation records
     */
    const std::vector<AllocationRecord>& allocations() const { return allocations_; }
    
    // --- Object Inspection ---
    
    /**
     * @brief Get object by ID
     */
    const HeapObject* getObject(int snapshotId, uint64_t objectId) const;
    
    /**
     * @brief Get retainers (path to GC root)
     */
    std::vector<HeapObject> getRetainers(int snapshotId, uint64_t objectId);
    
    /**
     * @brief Find objects by class
     */
    std::vector<HeapObject> findByClass(int snapshotId, const std::string& className);
    
    /**
     * @brief Search objects
     */
    std::vector<HeapObject> search(int snapshotId, const std::string& query);
    
    // --- GC ---
    
    /**
     * @brief Force garbage collection
     */
    void collectGarbage();
    
    // --- View Options ---
    
    enum class ViewMode {
        Summary,        // Statistics by constructor
        Containment,    // Object tree
        Statistics,     // Pie chart
        Comparison      // Diff between snapshots
    };
    
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return viewMode_; }
    
    // --- Export ---
    
    /**
     * @brief Export snapshot as JSON
     */
    std::string exportSnapshot(int id);
    
    /**
     * @brief Import snapshot
     */
    int importSnapshot(const std::string& json);
    
    // --- Callbacks ---
    void onSnapshot(SnapshotCallback callback);
    void onAllocation(AllocationCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
private:
    void renderSnapshotList();
    void renderSummaryView();
    void renderContainmentView();
    void renderStatisticsView();
    void renderComparisonView();
    void renderObjectDetails(uint64_t objectId);
    
    std::string formatSize(size_t bytes);
    
    std::vector<HeapSnapshot> snapshots_;
    std::vector<AllocationRecord> allocations_;
    
    int selectedSnapshotId_ = -1;
    int comparisonSnapshotId_ = -1;
    int nextSnapshotId_ = 1;
    bool tracking_ = false;
    
    ViewMode viewMode_ = ViewMode::Summary;
    std::string searchQuery_;
    
    std::chrono::system_clock::time_point trackStart_;
    
    std::vector<SnapshotCallback> snapshotCallbacks_;
    std::vector<AllocationCallback> allocationCallbacks_;
};

} // namespace Zepra::DevTools
