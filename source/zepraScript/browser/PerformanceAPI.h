/**
 * @file PerformanceAPI.h
 * @brief High-Resolution Timing and Performance API
 * 
 * Implements W3C Performance Timeline and User Timing APIs.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <memory>

namespace Zepra::Browser {

using Runtime::Value;

// =============================================================================
// Performance Entry Types
// =============================================================================

/**
 * @brief Base class for all performance entries
 */
class PerformanceEntry {
public:
    enum class EntryType {
        Mark,
        Measure,
        Navigation,
        Resource,
        Paint,
        LongestContentfulPaint,
        FirstInput,
        LayoutShift
    };
    
    PerformanceEntry(const std::string& name, EntryType type, double startTime, double duration = 0)
        : name_(name), entryType_(type), startTime_(startTime), duration_(duration) {}
    
    virtual ~PerformanceEntry() = default;
    
    const std::string& name() const { return name_; }
    EntryType entryType() const { return entryType_; }
    double startTime() const { return startTime_; }
    double duration() const { return duration_; }
    
    std::string entryTypeString() const {
        switch (entryType_) {
            case EntryType::Mark: return "mark";
            case EntryType::Measure: return "measure";
            case EntryType::Navigation: return "navigation";
            case EntryType::Resource: return "resource";
            case EntryType::Paint: return "paint";
            case EntryType::LongestContentfulPaint: return "largest-contentful-paint";
            case EntryType::FirstInput: return "first-input";
            case EntryType::LayoutShift: return "layout-shift";
        }
        return "unknown";
    }
    
protected:
    std::string name_;
    EntryType entryType_;
    double startTime_;
    double duration_;
};

// =============================================================================
// Performance Mark
// =============================================================================

/**
 * @brief User timing mark
 */
class PerformanceMark : public PerformanceEntry {
public:
    PerformanceMark(const std::string& name, double startTime)
        : PerformanceEntry(name, EntryType::Mark, startTime) {}
    
    // Optional detail data
    Value detail() const { return detail_; }
    void setDetail(const Value& detail) { detail_ = detail; }
    
private:
    Value detail_;
};

// =============================================================================
// Performance Measure
// =============================================================================

/**
 * @brief User timing measure (duration between two marks)
 */
class PerformanceMeasure : public PerformanceEntry {
public:
    PerformanceMeasure(const std::string& name, double startTime, double duration)
        : PerformanceEntry(name, EntryType::Measure, startTime, duration) {}
    
    Value detail() const { return detail_; }
    void setDetail(const Value& detail) { detail_ = detail; }
    
private:
    Value detail_;
};

// =============================================================================
// Navigation Timing
// =============================================================================

/**
 * @brief Navigation timing metrics
 */
class PerformanceNavigationTiming : public PerformanceEntry {
public:
    PerformanceNavigationTiming()
        : PerformanceEntry("", EntryType::Navigation, 0) {}
    
    // Timing milestones
    double unloadEventStart = 0;
    double unloadEventEnd = 0;
    double redirectStart = 0;
    double redirectEnd = 0;
    double fetchStart = 0;
    double domainLookupStart = 0;
    double domainLookupEnd = 0;
    double connectStart = 0;
    double connectEnd = 0;
    double secureConnectionStart = 0;
    double requestStart = 0;
    double responseStart = 0;
    double responseEnd = 0;
    double domInteractive = 0;
    double domContentLoadedEventStart = 0;
    double domContentLoadedEventEnd = 0;
    double domComplete = 0;
    double loadEventStart = 0;
    double loadEventEnd = 0;
    
    // Transfer size
    size_t transferSize = 0;
    size_t encodedBodySize = 0;
    size_t decodedBodySize = 0;
    
    // Type
    enum class NavigationType { Navigate, Reload, BackForward, Prerender };
    NavigationType type = NavigationType::Navigate;
    uint16_t redirectCount = 0;
};

// =============================================================================
// Resource Timing
// =============================================================================

/**
 * @brief Resource loading timing
 */
class PerformanceResourceTiming : public PerformanceEntry {
public:
    PerformanceResourceTiming(const std::string& name, double startTime)
        : PerformanceEntry(name, EntryType::Resource, startTime) {}
    
    std::string initiatorType;  // "script", "img", "link", "fetch", etc.
    std::string nextHopProtocol;
    
    double workerStart = 0;
    double redirectStart = 0;
    double redirectEnd = 0;
    double fetchStart = 0;
    double domainLookupStart = 0;
    double domainLookupEnd = 0;
    double connectStart = 0;
    double connectEnd = 0;
    double secureConnectionStart = 0;
    double requestStart = 0;
    double responseStart = 0;
    double responseEnd = 0;
    
    size_t transferSize = 0;
    size_t encodedBodySize = 0;
    size_t decodedBodySize = 0;
};

// =============================================================================
// Performance Observer
// =============================================================================

/**
 * @brief Options for PerformanceObserver
 */
struct PerformanceObserverInit {
    std::vector<std::string> entryTypes;  // Filter by entry type
    bool buffered = false;  // Include buffered entries
};

/**
 * @brief Entry list passed to observer callback
 */
class PerformanceObserverEntryList {
public:
    std::vector<const PerformanceEntry*> getEntries() const { return entries_; }
    
    std::vector<const PerformanceEntry*> getEntriesByType(const std::string& type) const {
        std::vector<const PerformanceEntry*> result;
        for (const auto* entry : entries_) {
            if (entry->entryTypeString() == type) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    std::vector<const PerformanceEntry*> getEntriesByName(const std::string& name) const {
        std::vector<const PerformanceEntry*> result;
        for (const auto* entry : entries_) {
            if (entry->name() == name) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    void add(const PerformanceEntry* entry) { entries_.push_back(entry); }
    
private:
    std::vector<const PerformanceEntry*> entries_;
};

/**
 * @brief Observes performance entries
 */
class PerformanceObserver {
public:
    using Callback = std::function<void(const PerformanceObserverEntryList&, PerformanceObserver*)>;
    
    explicit PerformanceObserver(Callback callback);
    ~PerformanceObserver();
    
    void observe(const PerformanceObserverInit& options);
    void disconnect();
    PerformanceObserverEntryList takeRecords();
    
    // Static: Get supported entry types
    static std::vector<std::string> supportedEntryTypes();
    
private:
    Callback callback_;
    PerformanceObserverInit options_;
    std::vector<const PerformanceEntry*> buffer_;
};

// =============================================================================
// Performance (main interface)
// =============================================================================

/**
 * @brief Main Performance interface (window.performance)
 */
class Performance {
public:
    static Performance& instance();
    
    /**
     * @brief High-resolution timestamp (milliseconds since navigation start)
     */
    double now() const;
    
    /**
     * @brief Time origin (absolute time in ms since epoch)
     */
    double timeOrigin() const { return timeOrigin_; }
    
    // =========================================================================
    // User Timing API
    // =========================================================================
    
    /**
     * @brief Create a performance mark
     */
    PerformanceMark* mark(const std::string& name);
    
    /**
     * @brief Create a measure between two marks or timestamps
     */
    PerformanceMeasure* measure(const std::string& name,
                                 const std::string& startMark = "",
                                 const std::string& endMark = "");
    
    /**
     * @brief Clear marks by name (or all if empty)
     */
    void clearMarks(const std::string& name = "");
    
    /**
     * @brief Clear measures by name (or all if empty)
     */
    void clearMeasures(const std::string& name = "");
    
    // =========================================================================
    // Entry Access
    // =========================================================================
    
    std::vector<const PerformanceEntry*> getEntries() const;
    std::vector<const PerformanceEntry*> getEntriesByType(const std::string& type) const;
    std::vector<const PerformanceEntry*> getEntriesByName(const std::string& name) const;
    
    // =========================================================================
    // Navigation Timing
    // =========================================================================
    
    const PerformanceNavigationTiming* navigation() const { return &navigation_; }
    
    // =========================================================================
    // Resource Timing
    // =========================================================================
    
    void clearResourceTimings();
    void setResourceTimingBufferSize(size_t maxSize);
    
    // Internal: add resource timing
    void addResourceTiming(PerformanceResourceTiming* entry);
    
    // =========================================================================
    // Observer Management
    // =========================================================================
    
    void registerObserver(PerformanceObserver* observer);
    void unregisterObserver(PerformanceObserver* observer);
    
private:
    Performance();
    
    void notifyObservers(const PerformanceEntry* entry);
    
    double timeOrigin_;
    std::chrono::steady_clock::time_point startTime_;
    
    PerformanceNavigationTiming navigation_;
    std::vector<std::unique_ptr<PerformanceEntry>> entries_;
    std::vector<PerformanceObserver*> observers_;
    
    // Mark lookup for measure()
    std::unordered_map<std::string, double> markTimestamps_;
    
    size_t resourceTimingBufferSize_ = 250;
};

// =============================================================================
// Builtin Functions
// =============================================================================

void initPerformance();
Value performanceNow(void* ctx, const std::vector<Value>& args);
Value performanceMark(void* ctx, const std::vector<Value>& args);
Value performanceMeasure(void* ctx, const std::vector<Value>& args);

} // namespace Zepra::Browser
