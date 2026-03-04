/**
 * @file timeline.cpp
 * @brief Performance Timeline and Measurement API
 *
 * Implements the Performance Timeline level 2 specification.
 * It provides mechanisms for tracing critical execution phases, measuring 
 * durations, and marking marks, which can be sent to devtools.
 * 
 * Includes: PerformanceMark, PerformanceMeasure, PerformanceObserver
 *
 * Ref: W3C Performance Timeline, WebKit Performance API
 */

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cstdio>

namespace Zepra::Profiler {

// =============================================================================
// Performance Entry
// =============================================================================

enum class EntryType {
    Mark,
    Measure,
    Resource,
    Navigation,
    Paint,
    Layout,
    Function
};

const char* entryTypeToString(EntryType type) {
    switch (type) {
        case EntryType::Mark: return "mark";
        case EntryType::Measure: return "measure";
        case EntryType::Resource: return "resource";
        case EntryType::Navigation: return "navigation";
        case EntryType::Paint: return "paint";
        case EntryType::Layout: return "layout";
        case EntryType::Function: return "function";
        default: return "unknown";
    }
}

struct PerformanceEntry {
    std::string name;
    EntryType type;
    double startTime; // In milliseconds from time origin
    double duration;  // In milliseconds
    std::string detail;
};

// =============================================================================
// PerformanceObserver
// =============================================================================

class PerformanceObserver {
public:
    using Callback = std::function<void(const std::vector<PerformanceEntry>&)>;

    PerformanceObserver(Callback cb) : callback_(std::move(cb)) {}

    void observe(const std::vector<EntryType>& types) {
        typesToObserve_ = types;
    }

    void disconnect() {
        typesToObserve_.clear();
    }

    bool observes(EntryType type) const {
        return std::find(typesToObserve_.begin(), typesToObserve_.end(), type) != typesToObserve_.end();
    }

    void notify(const std::vector<PerformanceEntry>& entries) {
        callback_(entries);
    }

private:
    Callback callback_;
    std::vector<EntryType> typesToObserve_;
};

// =============================================================================
// Performance Timeline
// =============================================================================

class PerformanceTimeline {
public:
    static PerformanceTimeline& instance() {
        static PerformanceTimeline inst;
        return inst;
    }

    PerformanceTimeline() {
        timeOrigin_ = std::chrono::steady_clock::now();
    }

    /**
     * Get current high-resolution time in milliseconds relative to origin.
     */
    double now() const {
        auto current = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> elapsed = current - timeOrigin_;
        return elapsed.count();
    }

    void mark(const std::string& name, const std::string& detail = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        PerformanceEntry entry;
        entry.name = name;
        entry.type = EntryType::Mark;
        entry.startTime = now();
        entry.duration = 0.0;
        entry.detail = detail;

        entries_.push_back(entry);
        notifyObservers(entry);
    }

    void measure(const std::string& name, const std::string& startMark, const std::string& endMark = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        double start = 0;
        if (!startMark.empty()) {
            auto it = std::find_if(entries_.rbegin(), entries_.rend(), [&](const PerformanceEntry& e) {
                return e.name == startMark && e.type == EntryType::Mark;
            });
            if (it != entries_.rend()) {
                start = it->startTime;
            }
        }

        double end = now();
        if (!endMark.empty()) {
            auto it = std::find_if(entries_.rbegin(), entries_.rend(), [&](const PerformanceEntry& e) {
                return e.name == endMark && e.type == EntryType::Mark;
            });
            if (it != entries_.rend()) {
                end = it->startTime;
            }
        }

        PerformanceEntry entry;
        entry.name = name;
        entry.type = EntryType::Measure;
        entry.startTime = start;
        entry.duration = end - start;
        entry.detail = "";

        entries_.push_back(entry);
        notifyObservers(entry);
    }

    void addEntry(const PerformanceEntry& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back(entry);
        notifyObservers(entry);
    }

    std::vector<PerformanceEntry> getEntries() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

    std::vector<PerformanceEntry> getEntriesByName(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<PerformanceEntry> result;
        for (const auto& entry : entries_) {
            if (entry.name == name) {
                result.push_back(entry);
            }
        }
        return result;
    }

    std::vector<PerformanceEntry> getEntriesByType(EntryType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<PerformanceEntry> result;
        for (const auto& entry : entries_) {
            if (entry.type == type) {
                result.push_back(entry);
            }
        }
        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }

    // Observer management
    void registerObserver(PerformanceObserver* observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_.push_back(observer);
    }

    void unregisterObserver(PerformanceObserver* observer) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(observers_.begin(), observers_.end(), observer);
        if (it != observers_.end()) {
            observers_.erase(it);
        }
    }

    void dump() const {
        std::lock_guard<std::mutex> lock(mutex_);
        printf("--- Performance Timeline ---\n");
        for (const auto& e : entries_) {
            printf("[%8.2f ms] %-10s %-20s (duration: %.2f ms)\n", 
                   e.startTime, entryTypeToString(e.type), e.name.c_str(), e.duration);
        }
    }

private:
    void notifyObservers(const PerformanceEntry& entry) {
        for (auto* obs : observers_) {
            if (obs->observes(entry.type)) {
                // In spec, notifications are batch queued, but for C++ API we notify immediately
                obs->notify({entry});
            }
        }
    }

    std::chrono::time_point<std::chrono::steady_clock> timeOrigin_;
    std::vector<PerformanceEntry> entries_;
    std::vector<PerformanceObserver*> observers_;
    mutable std::mutex mutex_;
};

} // namespace Zepra::Profiler
