/**
 * @file html_performance.hpp
 * @brief Performance APIs
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Performance entry types
 */
enum class PerformanceEntryType {
    Frame,
    Navigation,
    Resource,
    Mark,
    Measure,
    Paint,
    LongtTask,
    Element,
    LargestContentfulPaint,
    LayoutShift,
    FirstInput
};

/**
 * @brief Performance entry
 */
struct PerformanceEntry {
    std::string name;
    PerformanceEntryType entryType;
    double startTime = 0;
    double duration = 0;
};

/**
 * @brief Performance timing
 */
struct PerformanceTiming {
    double navigationStart = 0;
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
    double domLoading = 0;
    double domInteractive = 0;
    double domContentLoadedEventStart = 0;
    double domContentLoadedEventEnd = 0;
    double domComplete = 0;
    double loadEventStart = 0;
    double loadEventEnd = 0;
};

/**
 * @brief Performance navigation
 */
struct PerformanceNavigation {
    enum Type { Navigate = 0, Reload = 1, BackForward = 2, Prerender = 255 };
    Type type = Navigate;
    unsigned short redirectCount = 0;
};

/**
 * @brief Performance mark
 */
struct PerformanceMark : PerformanceEntry {
    std::any detail;
};

/**
 * @brief Performance measure
 */
struct PerformanceMeasure : PerformanceEntry {
    std::any detail;
};

/**
 * @brief Performance observer callback
 */
using PerformanceObserverCallback = std::function<void(
    const std::vector<PerformanceEntry>&)>;

/**
 * @brief Performance observer
 */
class PerformanceObserver {
public:
    explicit PerformanceObserver(PerformanceObserverCallback callback);
    ~PerformanceObserver();
    
    void observe(const std::vector<PerformanceEntryType>& types);
    void disconnect();
    std::vector<PerformanceEntry> takeRecords();
    
private:
    PerformanceObserverCallback callback_;
    std::vector<PerformanceEntryType> observedTypes_;
    std::vector<PerformanceEntry> buffer_;
};

/**
 * @brief Performance API
 */
class Performance {
public:
    Performance() = default;
    ~Performance() = default;
    
    // High-resolution time
    double now() const;
    double timeOrigin() const { return timeOrigin_; }
    
    // Timing
    const PerformanceTiming& timing() const { return timing_; }
    const PerformanceNavigation& navigation() const { return navigation_; }
    
    // Entries
    std::vector<PerformanceEntry> getEntries() const;
    std::vector<PerformanceEntry> getEntriesByType(PerformanceEntryType type) const;
    std::vector<PerformanceEntry> getEntriesByName(const std::string& name) const;
    
    // Marks and measures
    void mark(const std::string& name);
    void measure(const std::string& name,
                 const std::string& startMark = "",
                 const std::string& endMark = "");
    void clearMarks(const std::string& name = "");
    void clearMeasures(const std::string& name = "");
    
    // Memory (non-standard but useful)
    struct MemoryInfo {
        size_t totalJSHeapSize = 0;
        size_t usedJSHeapSize = 0;
        size_t jsHeapSizeLimit = 0;
    };
    MemoryInfo memory() const;
    
private:
    double timeOrigin_ = 0;
    PerformanceTiming timing_;
    PerformanceNavigation navigation_;
    std::vector<PerformanceEntry> entries_;
};

} // namespace Zepra::WebCore
