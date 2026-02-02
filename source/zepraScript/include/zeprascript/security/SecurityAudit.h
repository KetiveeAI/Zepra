/**
 * @file SecurityAudit.h
 * @brief Security Audit Infrastructure
 * 
 * Implements:
 * - Use-after-free detection
 * - Reference lifetime tracking
 * - Memory contract enforcement
 * - Exception state validation
 * - Cross-realm leak detection
 */

#pragma once

#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

namespace Zepra::Security {

// =============================================================================
// Object Lifetime Tracking
// =============================================================================

/**
 * @brief Tracks allocated objects for use-after-free detection
 */
class ObjectTracker {
public:
    static ObjectTracker& instance() {
        static ObjectTracker inst;
        return inst;
    }
    
    // Record allocation
    void recordAlloc(void* ptr, size_t size, const char* typeName) {
        std::lock_guard<std::mutex> lock(mutex_);
        AllocInfo info{size, typeName, true};
        allocations_[ptr] = info;
        stats_.totalAllocations++;
        stats_.currentLiveObjects++;
    }
    
    // Record deallocation
    void recordFree(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            it->second.alive = false;
            stats_.totalFrees++;
            stats_.currentLiveObjects--;
            freedAddresses_.insert(ptr);
        }
    }
    
    // Check if pointer is valid (not freed)
    bool isValidPointer(void* ptr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) return false;
        return it->second.alive;
    }
    
    // Check for use-after-free
    bool isUseAfterFree(void* ptr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return freedAddresses_.count(ptr) > 0;
    }
    
    // Assert valid access
    void assertValid(void* ptr, const char* location) {
        if (!isValidPointer(ptr)) {
            if (isUseAfterFree(ptr)) {
                reportViolation("USE-AFTER-FREE", ptr, location);
            } else {
                reportViolation("INVALID-ACCESS", ptr, location);
            }
        }
    }
    
    struct Stats {
        size_t totalAllocations = 0;
        size_t totalFrees = 0;
        size_t currentLiveObjects = 0;
        size_t violations = 0;
    };
    
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    void setViolationHandler(std::function<void(const std::string&)> handler) {
        violationHandler_ = std::move(handler);
    }
    
private:
    struct AllocInfo {
        size_t size;
        const char* typeName;
        bool alive;
    };
    
    void reportViolation(const char* type, void* ptr, const char* location) {
        stats_.violations++;
        if (violationHandler_) {
            std::string msg = std::string(type) + " at " + location + 
                             " ptr=" + std::to_string(reinterpret_cast<uintptr_t>(ptr));
            violationHandler_(msg);
        }
    }
    
    ObjectTracker() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocInfo> allocations_;
    std::unordered_set<void*> freedAddresses_;
    Stats stats_;
    std::function<void(const std::string&)> violationHandler_;
};

// Macros for instrumentation
#ifdef ZEPRA_SECURITY_AUDIT
    #define ZEPRA_TRACK_ALLOC(ptr, size, type) \
        Zepra::Security::ObjectTracker::instance().recordAlloc(ptr, size, #type)
    #define ZEPRA_TRACK_FREE(ptr) \
        Zepra::Security::ObjectTracker::instance().recordFree(ptr)
    #define ZEPRA_ASSERT_VALID(ptr) \
        Zepra::Security::ObjectTracker::instance().assertValid(ptr, __FUNCTION__)
#else
    #define ZEPRA_TRACK_ALLOC(ptr, size, type) ((void)0)
    #define ZEPRA_TRACK_FREE(ptr) ((void)0)
    #define ZEPRA_ASSERT_VALID(ptr) ((void)0)
#endif

// =============================================================================
// Reference Lifetime Tracking
// =============================================================================

/**
 * @brief Tracks reference counts and lifetimes
 */
class RefTracker {
public:
    static RefTracker& instance() {
        static RefTracker inst;
        return inst;
    }
    
    // Add reference
    void addRef(void* ptr, const char* source) {
        std::lock_guard<std::mutex> lock(mutex_);
        refCounts_[ptr]++;
        if (traceRefs_) {
            refHistory_[ptr].push_back({true, source});
        }
    }
    
    // Release reference
    void release(void* ptr, const char* source) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = refCounts_.find(ptr);
        if (it != refCounts_.end()) {
            if (it->second == 0) {
                reportViolation("DOUBLE-FREE-REF", ptr, source);
                return;
            }
            it->second--;
            if (traceRefs_) {
                refHistory_[ptr].push_back({false, source});
            }
        } else {
            reportViolation("RELEASE-UNTRACKED", ptr, source);
        }
    }
    
    // Get current ref count
    size_t refCount(void* ptr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = refCounts_.find(ptr);
        return it != refCounts_.end() ? it->second : 0;
    }
    
    // Check for leaks
    std::vector<void*> detectLeaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<void*> leaks;
        for (const auto& [ptr, count] : refCounts_) {
            if (count > 0) {
                leaks.push_back(ptr);
            }
        }
        return leaks;
    }
    
    void enableTracing(bool enable) { traceRefs_ = enable; }
    
private:
    struct RefEvent {
        bool isAdd;
        const char* source;
    };
    
    void reportViolation(const char* type, void* ptr, const char* source) {
        // Log violation
        (void)type; (void)ptr; (void)source;
    }
    
    RefTracker() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<void*, size_t> refCounts_;
    std::unordered_map<void*, std::vector<RefEvent>> refHistory_;
    bool traceRefs_ = false;
};

// =============================================================================
// Exception State Validator
// =============================================================================

/**
 * @brief Validates exception state consistency
 */
class ExceptionStateValidator {
public:
    struct ExceptionFrame {
        bool hasException = false;
        void* exceptionValue = nullptr;
        size_t stackDepth = 0;
    };
    
    // Push try block
    void pushTry(size_t stackDepth) {
        frames_.push_back({false, nullptr, stackDepth});
    }
    
    // Pop try block
    bool popTry() {
        if (frames_.empty()) {
            reportViolation("POP-EMPTY-EXCEPTION-STACK");
            return false;
        }
        frames_.pop_back();
        return true;
    }
    
    // Record exception
    void recordException(void* value) {
        if (!frames_.empty()) {
            frames_.back().hasException = true;
            frames_.back().exceptionValue = value;
        }
    }
    
    // Clear exception (after catch)
    void clearException() {
        if (!frames_.empty()) {
            frames_.back().hasException = false;
            frames_.back().exceptionValue = nullptr;
        }
    }
    
    // Validate state
    bool validateState(size_t expectedStackDepth) const {
        for (const auto& frame : frames_) {
            if (frame.hasException && frame.stackDepth > expectedStackDepth) {
                return false;  // Dangling exception
            }
        }
        return true;
    }
    
    // Check for leaking exceptions
    bool hasLeakingException() const {
        for (const auto& frame : frames_) {
            if (frame.hasException) {
                return true;
            }
        }
        return false;
    }
    
private:
    void reportViolation(const char* type) {
        (void)type;
    }
    
    std::vector<ExceptionFrame> frames_;
};

// =============================================================================
// Cross-Realm Validator
// =============================================================================

using RealmId = uint32_t;

/**
 * @brief Validates realm boundaries
 */
class RealmValidator {
public:
    // Tag object with realm
    void tagRealm(void* obj, RealmId realm) {
        std::lock_guard<std::mutex> lock(mutex_);
        objectRealms_[obj] = realm;
    }
    
    // Get object's realm
    RealmId getRealm(void* obj) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = objectRealms_.find(obj);
        return it != objectRealms_.end() ? it->second : 0;
    }
    
    // Check cross-realm access
    bool checkAccess(void* obj, RealmId accessingRealm) const {
        RealmId objectRealm = getRealm(obj);
        if (objectRealm == 0) return true;  // Untagged, allow
        return objectRealm == accessingRealm;
    }
    
    // Report cross-realm leak
    void reportLeak(void* obj, RealmId from, RealmId to) {
        leaks_.push_back({obj, from, to});
    }
    
    struct LeakInfo {
        void* object;
        RealmId sourceRealm;
        RealmId targetRealm;
    };
    
    const std::vector<LeakInfo>& getLeaks() const { return leaks_; }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, RealmId> objectRealms_;
    std::vector<LeakInfo> leaks_;
};

// =============================================================================
// Memory Bounds Checker
// =============================================================================

/**
 * @brief Validates memory access bounds
 */
class BoundsChecker {
public:
    struct Region {
        uintptr_t start;
        size_t size;
        bool readable;
        bool writable;
    };
    
    void registerRegion(void* ptr, size_t size, bool read, bool write) {
        regions_.push_back({
            reinterpret_cast<uintptr_t>(ptr),
            size,
            read,
            write
        });
    }
    
    void unregisterRegion(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        regions_.erase(
            std::remove_if(regions_.begin(), regions_.end(),
                          [addr](const Region& r) { return r.start == addr; }),
            regions_.end()
        );
    }
    
    bool checkRead(void* ptr, size_t len) const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        for (const auto& r : regions_) {
            if (addr >= r.start && addr + len <= r.start + r.size) {
                return r.readable;
            }
        }
        return false;  // Not in any known region
    }
    
    bool checkWrite(void* ptr, size_t len) const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        for (const auto& r : regions_) {
            if (addr >= r.start && addr + len <= r.start + r.size) {
                return r.writable;
            }
        }
        return false;
    }
    
private:
    std::vector<Region> regions_;
};

// =============================================================================
// Security Audit Report
// =============================================================================

/**
 * @brief Generates security audit report
 */
class AuditReport {
public:
    void addFinding(const std::string& category, const std::string& detail) {
        findings_.push_back({category, detail});
    }
    
    std::string generate() const {
        std::string report = "=== SECURITY AUDIT REPORT ===\n\n";
        
        // Object tracker stats
        auto stats = ObjectTracker::instance().stats();
        report += "Memory Safety:\n";
        report += "  Total allocations: " + std::to_string(stats.totalAllocations) + "\n";
        report += "  Total frees: " + std::to_string(stats.totalFrees) + "\n";
        report += "  Live objects: " + std::to_string(stats.currentLiveObjects) + "\n";
        report += "  Violations: " + std::to_string(stats.violations) + "\n\n";
        
        // Ref tracker leaks
        auto leaks = RefTracker::instance().detectLeaks();
        report += "Reference Leaks: " + std::to_string(leaks.size()) + "\n\n";
        
        // Findings
        report += "Findings (" + std::to_string(findings_.size()) + "):\n";
        for (const auto& f : findings_) {
            report += "  [" + f.category + "] " + f.detail + "\n";
        }
        
        return report;
    }
    
private:
    struct Finding {
        std::string category;
        std::string detail;
    };
    std::vector<Finding> findings_;
};

} // namespace Zepra::Security
