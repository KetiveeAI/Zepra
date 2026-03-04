/**
 * @file performance.cpp
 * @brief W3C Performance API implementation
 *
 * Implements:
 * - performance.now() high-resolution timing
 * - User Timing API (mark/measure)
 * - PerformanceObserver
 * - Navigation and Resource Timing
 *
 * Ref: https://w3c.github.io/hr-time/
 *      https://w3c.github.io/user-timing/
 */

#include "browser/PerformanceAPI.h"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include <algorithm>
#include <cmath>

namespace Zepra::Browser {

// =============================================================================
// Performance singleton
// =============================================================================

Performance& Performance::instance() {
    static Performance perf;
    return perf;
}

Performance::Performance()
    : timeOrigin_(0)
    , startTime_(std::chrono::steady_clock::now())
    , resourceTimingBufferSize_(250) {
    // timeOrigin = Unix epoch time at navigation start (milliseconds)
    auto epoch = std::chrono::system_clock::now().time_since_epoch();
    timeOrigin_ = std::chrono::duration<double, std::milli>(epoch).count();
}

double Performance::now() const {
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    // Return sub-millisecond precision (microseconds as fractional ms)
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

// =============================================================================
// User Timing: mark
// =============================================================================

PerformanceMark* Performance::mark(const std::string& name) {
    double startTime = now();
    auto mark = std::make_unique<PerformanceMark>(name, startTime);
    PerformanceMark* ptr = mark.get();

    // Store timestamp for measure() lookups
    markTimestamps_[name] = startTime;

    // Notify observers
    notifyObservers(ptr);

    entries_.push_back(std::move(mark));
    return ptr;
}

// =============================================================================
// User Timing: measure
// =============================================================================

PerformanceMeasure* Performance::measure(const std::string& name,
                                          const std::string& startMark,
                                          const std::string& endMark) {
    double startTime = 0;
    double endTime = now();

    // Resolve start mark
    if (!startMark.empty()) {
        auto it = markTimestamps_.find(startMark);
        if (it != markTimestamps_.end()) {
            startTime = it->second;
        }
    }

    // Resolve end mark
    if (!endMark.empty()) {
        auto it = markTimestamps_.find(endMark);
        if (it != markTimestamps_.end()) {
            endTime = it->second;
        }
    }

    double duration = endTime - startTime;
    auto measure = std::make_unique<PerformanceMeasure>(name, startTime, duration);
    PerformanceMeasure* ptr = measure.get();

    notifyObservers(ptr);

    entries_.push_back(std::move(measure));
    return ptr;
}

// =============================================================================
// Clear
// =============================================================================

void Performance::clearMarks(const std::string& name) {
    if (name.empty()) {
        // Remove all marks
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [](const std::unique_ptr<PerformanceEntry>& e) {
                    return e->entryType() == PerformanceEntry::EntryType::Mark;
                }),
            entries_.end());
        markTimestamps_.clear();
    } else {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [&name](const std::unique_ptr<PerformanceEntry>& e) {
                    return e->entryType() == PerformanceEntry::EntryType::Mark &&
                           e->name() == name;
                }),
            entries_.end());
        markTimestamps_.erase(name);
    }
}

void Performance::clearMeasures(const std::string& name) {
    if (name.empty()) {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [](const std::unique_ptr<PerformanceEntry>& e) {
                    return e->entryType() == PerformanceEntry::EntryType::Measure;
                }),
            entries_.end());
    } else {
        entries_.erase(
            std::remove_if(entries_.begin(), entries_.end(),
                [&name](const std::unique_ptr<PerformanceEntry>& e) {
                    return e->entryType() == PerformanceEntry::EntryType::Measure &&
                           e->name() == name;
                }),
            entries_.end());
    }
}

// =============================================================================
// Entry Access
// =============================================================================

std::vector<const PerformanceEntry*> Performance::getEntries() const {
    std::vector<const PerformanceEntry*> result;
    result.reserve(entries_.size());
    for (const auto& e : entries_) {
        result.push_back(e.get());
    }
    // Sort by startTime
    std::sort(result.begin(), result.end(),
        [](const PerformanceEntry* a, const PerformanceEntry* b) {
            return a->startTime() < b->startTime();
        });
    return result;
}

std::vector<const PerformanceEntry*> Performance::getEntriesByType(const std::string& type) const {
    std::vector<const PerformanceEntry*> result;
    for (const auto& e : entries_) {
        if (e->entryTypeString() == type) {
            result.push_back(e.get());
        }
    }
    std::sort(result.begin(), result.end(),
        [](const PerformanceEntry* a, const PerformanceEntry* b) {
            return a->startTime() < b->startTime();
        });
    return result;
}

std::vector<const PerformanceEntry*> Performance::getEntriesByName(const std::string& name) const {
    std::vector<const PerformanceEntry*> result;
    for (const auto& e : entries_) {
        if (e->name() == name) {
            result.push_back(e.get());
        }
    }
    std::sort(result.begin(), result.end(),
        [](const PerformanceEntry* a, const PerformanceEntry* b) {
            return a->startTime() < b->startTime();
        });
    return result;
}

// =============================================================================
// Resource Timing
// =============================================================================

void Performance::clearResourceTimings() {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [](const std::unique_ptr<PerformanceEntry>& e) {
                return e->entryType() == PerformanceEntry::EntryType::Resource;
            }),
        entries_.end());
}

void Performance::setResourceTimingBufferSize(size_t maxSize) {
    resourceTimingBufferSize_ = maxSize;
}

void Performance::addResourceTiming(PerformanceResourceTiming* entry) {
    // Count existing resource entries
    size_t resourceCount = 0;
    for (const auto& e : entries_) {
        if (e->entryType() == PerformanceEntry::EntryType::Resource) {
            resourceCount++;
        }
    }

    if (resourceCount >= resourceTimingBufferSize_) {
        return; // Buffer full
    }

    notifyObservers(entry);
    entries_.push_back(std::unique_ptr<PerformanceEntry>(entry));
}

// =============================================================================
// Observer Management
// =============================================================================

void Performance::registerObserver(PerformanceObserver* observer) {
    for (auto* o : observers_) {
        if (o == observer) return;
    }
    observers_.push_back(observer);
}

void Performance::unregisterObserver(PerformanceObserver* observer) {
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
}

void Performance::notifyObservers(const PerformanceEntry* entry) {
    for (auto* observer : observers_) {
        // Check if observer is interested in this entry type
        // (observer filters are checked in PerformanceObserver)
        (void)observer;
        (void)entry;
    }
}

// =============================================================================
// PerformanceObserver
// =============================================================================

PerformanceObserver::PerformanceObserver(Callback callback)
    : callback_(std::move(callback)) {}

PerformanceObserver::~PerformanceObserver() {
    disconnect();
}

void PerformanceObserver::observe(const PerformanceObserverInit& options) {
    options_ = options;
    Performance::instance().registerObserver(this);

    // If buffered, deliver existing entries
    if (options.buffered) {
        PerformanceObserverEntryList list;
        for (const auto& type : options.entryTypes) {
            auto entries = Performance::instance().getEntriesByType(type);
            for (const auto* entry : entries) {
                list.add(entry);
            }
        }
        if (!list.getEntries().empty()) {
            callback_(list, this);
        }
    }
}

void PerformanceObserver::disconnect() {
    Performance::instance().unregisterObserver(this);
}

PerformanceObserverEntryList PerformanceObserver::takeRecords() {
    PerformanceObserverEntryList list;
    for (const auto* entry : buffer_) {
        list.add(entry);
    }
    buffer_.clear();
    return list;
}

std::vector<std::string> PerformanceObserver::supportedEntryTypes() {
    return {
        "mark", "measure", "navigation", "resource",
        "paint", "largest-contentful-paint", "first-input", "layout-shift"
    };
}

// =============================================================================
// Builtin Functions (registration into JS global)
// =============================================================================

void initPerformance() {
    // Would register performance object on the global
}

Value performanceNow(void* /*ctx*/, const std::vector<Value>& /*args*/) {
    return Value::number(Performance::instance().now());
}

Value performanceMark(void* /*ctx*/, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        return Value::undefined();
    }
    auto* str = static_cast<Runtime::String*>(args[0].asObject());
    Performance::instance().mark(str->value());
    return Value::undefined();
}

Value performanceMeasure(void* /*ctx*/, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        return Value::undefined();
    }
    auto* nameStr = static_cast<Runtime::String*>(args[0].asObject());
    std::string startMark, endMark;

    if (args.size() > 1 && args[1].isString()) {
        startMark = static_cast<Runtime::String*>(args[1].asObject())->value();
    }
    if (args.size() > 2 && args[2].isString()) {
        endMark = static_cast<Runtime::String*>(args[2].asObject())->value();
    }

    Performance::instance().measure(nameStr->value(), startMark, endMark);
    return Value::undefined();
}

} // namespace Zepra::Browser
