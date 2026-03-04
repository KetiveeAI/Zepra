/**
 * @file PerformanceAPI.h
 * @brief Performance API Implementation
 * 
 * High Resolution Time and Performance Timeline:
 * - performance.now()
 * - PerformanceEntry, PerformanceMark, PerformanceMeasure
 * - PerformanceObserver
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <memory>

namespace Zepra::API {

// =============================================================================
// PerformanceEntry
// =============================================================================

/**
 * @brief Base performance entry
 */
class PerformanceEntry {
public:
    PerformanceEntry(const std::string& name, const std::string& entryType,
                     double startTime, double duration)
        : name_(name), entryType_(entryType)
        , startTime_(startTime), duration_(duration) {}
    
    virtual ~PerformanceEntry() = default;
    
    const std::string& name() const { return name_; }
    const std::string& entryType() const { return entryType_; }
    double startTime() const { return startTime_; }
    double duration() const { return duration_; }
    
    // To JSON
    virtual std::string toJSON() const {
        return "{\"name\":\"" + name_ + "\","
               "\"entryType\":\"" + entryType_ + "\","
               "\"startTime\":" + std::to_string(startTime_) + ","
               "\"duration\":" + std::to_string(duration_) + "}";
    }
    
protected:
    std::string name_;
    std::string entryType_;
    double startTime_;
    double duration_;
};

// =============================================================================
// PerformanceMark
// =============================================================================

/**
 * @brief User-defined timestamp marker
 */
class PerformanceMark : public PerformanceEntry {
public:
    PerformanceMark(const std::string& name, double startTime,
                    const std::string& detail = "")
        : PerformanceEntry(name, "mark", startTime, 0)
        , detail_(detail) {}
    
    const std::string& detail() const { return detail_; }
    
private:
    std::string detail_;
};

// =============================================================================
// PerformanceMeasure
// =============================================================================

/**
 * @brief Measurement between two marks
 */
class PerformanceMeasure : public PerformanceEntry {
public:
    PerformanceMeasure(const std::string& name, double startTime, double duration,
                       const std::string& detail = "")
        : PerformanceEntry(name, "measure", startTime, duration)
        , detail_(detail) {}
    
    const std::string& detail() const { return detail_; }
    
private:
    std::string detail_;
};

// =============================================================================
// PerformanceResourceTiming
// =============================================================================

/**
 * @brief Resource loading timing
 */
class PerformanceResourceTiming : public PerformanceEntry {
public:
    PerformanceResourceTiming(const std::string& name, double startTime)
        : PerformanceEntry(name, "resource", startTime, 0) {}
    
    // Timing attributes
    double fetchStart = 0;
    double domainLookupStart = 0;
    double domainLookupEnd = 0;
    double connectStart = 0;
    double connectEnd = 0;
    double requestStart = 0;
    double responseStart = 0;
    double responseEnd = 0;
    
    size_t transferSize = 0;
    size_t encodedBodySize = 0;
    size_t decodedBodySize = 0;
};

// =============================================================================
// PerformanceObserverEntryList
// =============================================================================

/**
 * @brief List of observed entries
 */
class PerformanceObserverEntryList {
public:
    void add(std::shared_ptr<PerformanceEntry> entry) {
        entries_.push_back(entry);
    }
    
    const std::vector<std::shared_ptr<PerformanceEntry>>& getEntries() const {
        return entries_;
    }
    
    std::vector<std::shared_ptr<PerformanceEntry>> getEntriesByType(
            const std::string& type) const {
        std::vector<std::shared_ptr<PerformanceEntry>> result;
        for (const auto& entry : entries_) {
            if (entry->entryType() == type) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    std::vector<std::shared_ptr<PerformanceEntry>> getEntriesByName(
            const std::string& name) const {
        std::vector<std::shared_ptr<PerformanceEntry>> result;
        for (const auto& entry : entries_) {
            if (entry->name() == name) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
private:
    std::vector<std::shared_ptr<PerformanceEntry>> entries_;
};

// =============================================================================
// PerformanceObserver
// =============================================================================

class Performance;

/**
 * @brief Observes performance entries
 */
class PerformanceObserver {
public:
    using Callback = std::function<void(const PerformanceObserverEntryList&)>;
    
    explicit PerformanceObserver(Callback callback)
        : callback_(std::move(callback)) {}
    
    void observe(const std::vector<std::string>& entryTypes) {
        observedTypes_ = entryTypes;
        registered_ = true;
    }
    
    void disconnect() {
        registered_ = false;
        observedTypes_.clear();
    }
    
    bool isRegistered() const { return registered_; }
    
    const std::vector<std::string>& observedTypes() const {
        return observedTypes_;
    }
    
    void notify(const PerformanceObserverEntryList& entries) {
        if (callback_) {
            callback_(entries);
        }
    }
    
private:
    Callback callback_;
    std::vector<std::string> observedTypes_;
    bool registered_ = false;
};

// =============================================================================
// Performance
// =============================================================================

/**
 * @brief Main performance interface
 */
class Performance {
public:
    Performance() {
        timeOrigin_ = std::chrono::steady_clock::now();
    }
    
    // High resolution timestamp
    double now() const {
        auto current = std::chrono::steady_clock::now();
        auto duration = current - timeOrigin_;
        return std::chrono::duration<double, std::milli>(duration).count();
    }
    
    // Time origin as Unix timestamp
    double timeOrigin() const {
        auto wallClock = std::chrono::system_clock::now();
        return std::chrono::duration<double, std::milli>(
            wallClock.time_since_epoch()).count();
    }
    
    // Marks
    PerformanceMark* mark(const std::string& name,
                          const std::string& detail = "") {
        auto mark = std::make_shared<PerformanceMark>(name, now(), detail);
        
        std::lock_guard lock(mutex_);
        marks_[name] = mark;
        entries_.push_back(mark);
        notifyObservers(mark);
        
        return mark.get();
    }
    
    void clearMarks(const std::string& name = "") {
        std::lock_guard lock(mutex_);
        if (name.empty()) {
            marks_.clear();
        } else {
            marks_.erase(name);
        }
    }
    
    // Measures
    PerformanceMeasure* measure(const std::string& name,
                                 const std::string& startMark = "",
                                 const std::string& endMark = "") {
        double startTime = 0;
        double endTime = now();
        
        {
            std::lock_guard lock(mutex_);
            if (!startMark.empty()) {
                auto it = marks_.find(startMark);
                if (it != marks_.end()) {
                    startTime = it->second->startTime();
                }
            }
            if (!endMark.empty()) {
                auto it = marks_.find(endMark);
                if (it != marks_.end()) {
                    endTime = it->second->startTime();
                }
            }
        }
        
        auto measure = std::make_shared<PerformanceMeasure>(
            name, startTime, endTime - startTime);
        
        std::lock_guard lock(mutex_);
        entries_.push_back(measure);
        notifyObservers(measure);
        
        return measure.get();
    }
    
    void clearMeasures(const std::string& name = "") {
        std::lock_guard lock(mutex_);
        if (name.empty()) {
            entries_.erase(
                std::remove_if(entries_.begin(), entries_.end(),
                    [](const auto& e) { return e->entryType() == "measure"; }),
                entries_.end());
        }
    }
    
    // Get entries
    std::vector<std::shared_ptr<PerformanceEntry>> getEntries() const {
        std::lock_guard lock(mutex_);
        return entries_;
    }
    
    std::vector<std::shared_ptr<PerformanceEntry>> getEntriesByType(
            const std::string& type) const {
        std::lock_guard lock(mutex_);
        std::vector<std::shared_ptr<PerformanceEntry>> result;
        for (const auto& entry : entries_) {
            if (entry->entryType() == type) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    std::vector<std::shared_ptr<PerformanceEntry>> getEntriesByName(
            const std::string& name) const {
        std::lock_guard lock(mutex_);
        std::vector<std::shared_ptr<PerformanceEntry>> result;
        for (const auto& entry : entries_) {
            if (entry->name() == name) {
                result.push_back(entry);
            }
        }
        return result;
    }
    
    // Observers
    void addObserver(std::shared_ptr<PerformanceObserver> observer) {
        std::lock_guard lock(mutex_);
        observers_.push_back(observer);
    }
    
    void removeObserver(std::shared_ptr<PerformanceObserver> observer) {
        std::lock_guard lock(mutex_);
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end());
    }
    
private:
    void notifyObservers(std::shared_ptr<PerformanceEntry> entry) {
        for (auto& observer : observers_) {
            if (!observer->isRegistered()) continue;
            
            for (const auto& type : observer->observedTypes()) {
                if (type == entry->entryType()) {
                    PerformanceObserverEntryList list;
                    list.add(entry);
                    observer->notify(list);
                    break;
                }
            }
        }
    }
    
    std::chrono::steady_clock::time_point timeOrigin_;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<PerformanceMark>> marks_;
    std::vector<std::shared_ptr<PerformanceEntry>> entries_;
    std::vector<std::shared_ptr<PerformanceObserver>> observers_;
};

// =============================================================================
// Global Performance
// =============================================================================

Performance& performance();

} // namespace Zepra::API
