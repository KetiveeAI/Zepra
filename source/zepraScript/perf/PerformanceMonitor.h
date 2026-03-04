/**
 * @file PerformanceMonitor.h
 * @brief Performance Hygiene Monitoring
 * 
 * Implements:
 * - Startup time tracking
 * - Memory growth monitoring
 * - GC pause measurement
 * - Cache efficiency tracking
 * - Pathological case detection
 */

#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace Zepra::Perf {

// =============================================================================
// Timing Utilities
// =============================================================================

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::nanoseconds;

inline double toMs(Duration d) {
    return std::chrono::duration<double, std::milli>(d).count();
}

// =============================================================================
// Startup Monitor
// =============================================================================

/**
 * @brief Tracks engine startup time
 */
class StartupMonitor {
public:
    void recordPhaseStart(const std::string& phase) {
        phases_[phase].start = Clock::now();
    }
    
    void recordPhaseEnd(const std::string& phase) {
        auto it = phases_.find(phase);
        if (it != phases_.end()) {
            it->second.end = Clock::now();
            it->second.completed = true;
        }
    }
    
    double phaseTimeMs(const std::string& phase) const {
        auto it = phases_.find(phase);
        if (it == phases_.end() || !it->second.completed) return -1;
        return toMs(it->second.end - it->second.start);
    }
    
    double totalStartupMs() const {
        double total = 0;
        for (const auto& [_, phase] : phases_) {
            if (phase.completed) {
                total += toMs(phase.end - phase.start);
            }
        }
        return total;
    }
    
    bool meetsTarget(double targetMs) const {
        return totalStartupMs() <= targetMs;
    }
    
    struct Report {
        double totalMs;
        std::vector<std::pair<std::string, double>> phases;
        bool targetMet;
    };
    
    Report generateReport(double targetMs) const {
        Report r;
        r.totalMs = totalStartupMs();
        r.targetMet = r.totalMs <= targetMs;
        
        for (const auto& [name, phase] : phases_) {
            if (phase.completed) {
                r.phases.push_back({name, toMs(phase.end - phase.start)});
            }
        }
        
        // Sort by duration descending
        std::sort(r.phases.begin(), r.phases.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return r;
    }
    
private:
    struct Phase {
        TimePoint start;
        TimePoint end;
        bool completed = false;
    };
    std::unordered_map<std::string, Phase> phases_;
};

// =============================================================================
// Memory Monitor
// =============================================================================

/**
 * @brief Tracks memory usage and growth
 */
class MemoryMonitor {
public:
    void recordSample(size_t heapUsed, size_t heapTotal) {
        samples_.push_back({Clock::now(), heapUsed, heapTotal});
        
        if (heapUsed > peakUsed_) peakUsed_ = heapUsed;
    }
    
    // Check if memory is stable (no unbounded growth)
    bool isStable(size_t windowSamples = 100) const {
        if (samples_.size() < windowSamples) return true;
        
        // Compare first and last quarter averages
        size_t quarter = windowSamples / 4;
        size_t start = samples_.size() - windowSamples;
        
        double earlyAvg = averageUsage(start, start + quarter);
        double lateAvg = averageUsage(samples_.size() - quarter, samples_.size());
        
        // Growth > 50% in window = unstable
        return lateAvg < earlyAvg * 1.5;
    }
    
    // Check for memory leak pattern
    bool hasLeakPattern() const {
        if (samples_.size() < 10) return false;
        
        // Linear regression on memory usage
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        size_t n = samples_.size();
        
        for (size_t i = 0; i < n; i++) {
            double x = static_cast<double>(i);
            double y = static_cast<double>(samples_[i].heapUsed);
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }
        
        double slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
        
        // Positive slope > threshold = potential leak
        return slope > 1000;  // More than 1KB/sample growth
    }
    
    size_t peakUsage() const { return peakUsed_; }
    
    size_t currentUsage() const {
        return samples_.empty() ? 0 : samples_.back().heapUsed;
    }
    
private:
    double averageUsage(size_t from, size_t to) const {
        if (to <= from) return 0;
        size_t sum = 0;
        for (size_t i = from; i < to && i < samples_.size(); i++) {
            sum += samples_[i].heapUsed;
        }
        return static_cast<double>(sum) / (to - from);
    }
    
    struct Sample {
        TimePoint time;
        size_t heapUsed;
        size_t heapTotal;
    };
    std::vector<Sample> samples_;
    size_t peakUsed_ = 0;
};

// =============================================================================
// GC Pause Monitor
// =============================================================================

/**
 * @brief Tracks GC pause durations
 */
class GCPauseMonitor {
public:
    void recordPause(Duration duration, bool isMinor) {
        double ms = toMs(duration);
        pauses_.push_back({ms, isMinor});
        
        if (isMinor) {
            minorPauses_++;
            totalMinorMs_ += ms;
        } else {
            majorPauses_++;
            totalMajorMs_ += ms;
        }
    }
    
    // Check if pauses are acceptable
    bool meetsTarget(double maxPauseMs) const {
        for (const auto& p : pauses_) {
            if (p.durationMs > maxPauseMs) return false;
        }
        return true;
    }
    
    double p99PauseMs() const {
        if (pauses_.empty()) return 0;
        
        std::vector<double> sorted;
        for (const auto& p : pauses_) {
            sorted.push_back(p.durationMs);
        }
        std::sort(sorted.begin(), sorted.end());
        
        size_t idx = static_cast<size_t>(sorted.size() * 0.99);
        return sorted[idx];
    }
    
    double averageMinorMs() const {
        return minorPauses_ > 0 ? totalMinorMs_ / minorPauses_ : 0;
    }
    
    double averageMajorMs() const {
        return majorPauses_ > 0 ? totalMajorMs_ / majorPauses_ : 0;
    }
    
    struct Stats {
        size_t minorCount;
        size_t majorCount;
        double avgMinorMs;
        double avgMajorMs;
        double p99Ms;
        double maxMs;
    };
    
    Stats stats() const {
        double maxMs = 0;
        for (const auto& p : pauses_) {
            if (p.durationMs > maxMs) maxMs = p.durationMs;
        }
        
        return {
            minorPauses_,
            majorPauses_,
            averageMinorMs(),
            averageMajorMs(),
            p99PauseMs(),
            maxMs
        };
    }
    
private:
    struct Pause {
        double durationMs;
        bool isMinor;
    };
    std::vector<Pause> pauses_;
    size_t minorPauses_ = 0;
    size_t majorPauses_ = 0;
    double totalMinorMs_ = 0;
    double totalMajorMs_ = 0;
};

// =============================================================================
// Cache Monitor
// =============================================================================

/**
 * @brief Tracks cache hit rates
 */
class CacheMonitor {
public:
    void recordHit(const std::string& cacheName) {
        caches_[cacheName].hits++;
    }
    
    void recordMiss(const std::string& cacheName) {
        caches_[cacheName].misses++;
    }
    
    double hitRate(const std::string& cacheName) const {
        auto it = caches_.find(cacheName);
        if (it == caches_.end()) return 0;
        
        size_t total = it->second.hits + it->second.misses;
        return total > 0 ? 
            static_cast<double>(it->second.hits) / total : 0;
    }
    
    bool meetsTarget(const std::string& cacheName, double targetRate) const {
        return hitRate(cacheName) >= targetRate;
    }
    
    struct CacheStats {
        std::string name;
        size_t hits;
        size_t misses;
        double hitRate;
    };
    
    std::vector<CacheStats> allStats() const {
        std::vector<CacheStats> result;
        for (const auto& [name, stats] : caches_) {
            size_t total = stats.hits + stats.misses;
            double rate = total > 0 ? 
                static_cast<double>(stats.hits) / total : 0;
            result.push_back({name, stats.hits, stats.misses, rate});
        }
        return result;
    }
    
private:
    struct CacheData {
        size_t hits = 0;
        size_t misses = 0;
    };
    std::unordered_map<std::string, CacheData> caches_;
};

// =============================================================================
// Pathological Case Detector
// =============================================================================

/**
 * @brief Detects pathological performance cases
 */
class PathologicalDetector {
public:
    // Record operation timing
    void recordOperation(const std::string& op, Duration duration) {
        operations_[op].push_back(toMs(duration));
    }
    
    // Check for operations taking much longer than average
    std::vector<std::string> detectOutliers(double stdDevThreshold = 3.0) const {
        std::vector<std::string> outliers;
        
        for (const auto& [op, timings] : operations_) {
            if (timings.size() < 10) continue;
            
            double mean = 0;
            for (double t : timings) mean += t;
            mean /= timings.size();
            
            double variance = 0;
            for (double t : timings) {
                variance += (t - mean) * (t - mean);
            }
            variance /= timings.size();
            double stdDev = std::sqrt(variance);
            
            // Check for outliers
            for (double t : timings) {
                if (t > mean + stdDevThreshold * stdDev) {
                    outliers.push_back(op);
                    break;
                }
            }
        }
        
        return outliers;
    }
    
    // Check for operations with high variance
    std::vector<std::string> detectUnstable(double cvThreshold = 0.5) const {
        std::vector<std::string> unstable;
        
        for (const auto& [op, timings] : operations_) {
            if (timings.size() < 10) continue;
            
            double mean = 0;
            for (double t : timings) mean += t;
            mean /= timings.size();
            
            double variance = 0;
            for (double t : timings) {
                variance += (t - mean) * (t - mean);
            }
            variance /= timings.size();
            double stdDev = std::sqrt(variance);
            
            // Coefficient of variation
            double cv = mean > 0 ? stdDev / mean : 0;
            if (cv > cvThreshold) {
                unstable.push_back(op);
            }
        }
        
        return unstable;
    }
    
private:
    std::unordered_map<std::string, std::vector<double>> operations_;
};

// =============================================================================
// Performance Dashboard
// =============================================================================

/**
 * @brief Aggregates all performance metrics
 */
class PerformanceDashboard {
public:
    StartupMonitor startup;
    MemoryMonitor memory;
    GCPauseMonitor gcPause;
    CacheMonitor cache;
    PathologicalDetector pathological;
    
    struct HealthCheck {
        bool startupOk;
        bool memoryStable;
        bool noLeaks;
        bool gcPausesOk;
        bool cacheEfficient;
        bool noPathological;
        
        bool allGreen() const {
            return startupOk && memoryStable && noLeaks &&
                   gcPausesOk && cacheEfficient && noPathological;
        }
    };
    
    HealthCheck runHealthCheck(
        double startupTargetMs = 50.0,
        double maxGCPauseMs = 10.0,
        double minCacheHitRate = 0.8
    ) {
        HealthCheck check;
        
        check.startupOk = startup.meetsTarget(startupTargetMs);
        check.memoryStable = memory.isStable();
        check.noLeaks = !memory.hasLeakPattern();
        check.gcPausesOk = gcPause.meetsTarget(maxGCPauseMs);
        
        auto cacheStats = cache.allStats();
        check.cacheEfficient = true;
        for (const auto& s : cacheStats) {
            if (s.hitRate < minCacheHitRate) {
                check.cacheEfficient = false;
                break;
            }
        }
        
        check.noPathological = pathological.detectOutliers().empty();
        
        return check;
    }
};

} // namespace Zepra::Perf
