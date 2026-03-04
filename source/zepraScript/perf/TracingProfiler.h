/**
 * @file TracingProfiler.h
 * @brief Detailed execution tracing profiler
 * 
 * Implements:
 * - Chrome Trace Event format output
 * - Function/event tracing
 * - Async operation tracking
 * - Memory allocation tracing
 * 
 * Compatible with chrome://tracing
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>

namespace Zepra::Perf {

// =============================================================================
// Trace Event Types (Chrome Trace Format)
// =============================================================================

enum class TraceEventPhase : char {
    Begin = 'B',            // Duration event begin
    End = 'E',              // Duration event end
    Complete = 'X',         // Complete event (with duration)
    Instant = 'i',          // Instant event
    Counter = 'C',          // Counter event
    AsyncBegin = 'b',       // Async event begin
    AsyncInstant = 'n',     // Async event instant
    AsyncEnd = 'e',         // Async event end
    FlowStart = 's',        // Flow event start
    FlowStep = 't',         // Flow event step
    FlowEnd = 'f',          // Flow event end
    Metadata = 'M',         // Metadata event
    MemoryDump = 'v',       // Memory dump
    Mark = 'R',             // Mark event
    ObjectCreated = 'N',    // Object created
    ObjectSnapshot = 'O',   // Object snapshot
    ObjectDestroyed = 'D'   // Object destroyed
};

// =============================================================================
// Trace Event
// =============================================================================

struct TraceEvent {
    std::string name;
    std::string category;
    TraceEventPhase phase;
    uint64_t timestamp;     // Microseconds since epoch
    uint64_t duration = 0;  // For complete events
    uint32_t pid;           // Process ID
    uint32_t tid;           // Thread ID
    
    // Optional fields
    std::string id;                         // For async events
    std::vector<std::pair<std::string, std::string>> args;
    
    // Stack trace (optional)
    std::vector<std::string> stack;
};

// =============================================================================
// Trace Categories
// =============================================================================

namespace TraceCategory {
    constexpr const char* JS_EXECUTION = "js";
    constexpr const char* GC = "gc";
    constexpr const char* COMPILE = "compile";
    constexpr const char* JIT = "jit";
    constexpr const char* PARSE = "parse";
    constexpr const char* NETWORK = "network";
    constexpr const char* ASYNC = "async";
    constexpr const char* MEMORY = "memory";
    constexpr const char* V8 = "v8";  // Compatibility
}

// =============================================================================
// Tracing Profiler
// =============================================================================

class TracingProfiler {
public:
    TracingProfiler() : pid_(getPid()), startTime_(std::chrono::steady_clock::now()) {}
    ~TracingProfiler() { stop(); }
    
    // =========================================================================
    // Control
    // =========================================================================
    
    void start(const std::string& outputPath = "");
    void stop();
    bool isRecording() const { return recording_.load(); }
    
    /**
     * @brief Flush events to disk
     */
    void flush();
    
    /**
     * @brief Clear recorded events
     */
    void clear();
    
    // =========================================================================
    // Event Recording
    // =========================================================================
    
    /**
     * @brief Begin duration event
     */
    void beginEvent(const std::string& name, const std::string& category = TraceCategory::JS_EXECUTION);
    
    /**
     * @brief End duration event
     */
    void endEvent(const std::string& name, const std::string& category = TraceCategory::JS_EXECUTION);
    
    /**
     * @brief Complete event (with known duration)
     */
    void completeEvent(const std::string& name, 
                       const std::string& category,
                       uint64_t durationUs);
    
    /**
     * @brief Instant event
     */
    void instantEvent(const std::string& name, 
                      const std::string& category = TraceCategory::JS_EXECUTION);
    
    /**
     * @brief Counter event
     */
    void counterEvent(const std::string& name, 
                      const std::string& series,
                      int64_t value);
    
    // =========================================================================
    // Async Events
    // =========================================================================
    
    void asyncBegin(const std::string& name, 
                    const std::string& id,
                    const std::string& category = TraceCategory::ASYNC);
    
    void asyncEnd(const std::string& name,
                  const std::string& id, 
                  const std::string& category = TraceCategory::ASYNC);
    
    // =========================================================================
    // Memory Events
    // =========================================================================
    
    void memorySnapshot(size_t heapUsed, size_t heapTotal);
    void gcEvent(const std::string& type, uint64_t durationUs, size_t freed);
    
    // =========================================================================
    // Metadata
    // =========================================================================
    
    void setProcessName(const std::string& name);
    void setThreadName(const std::string& name);
    
    // =========================================================================
    // Export
    // =========================================================================
    
    /**
     * @brief Export events to Chrome Trace JSON format
     */
    std::string exportToJSON() const;
    
    /**
     * @brief Write events to file
     */
    bool writeToFile(const std::string& path);
    
    /**
     * @brief Get event count
     */
    size_t eventCount() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size(); 
    }
    
private:
    mutable std::mutex mutex_;
    std::atomic<bool> recording_{false};
    
    std::vector<TraceEvent> events_;
    std::string outputPath_;
    std::ofstream outputFile_;
    
    uint32_t pid_;
    std::chrono::steady_clock::time_point startTime_;
    
    // Thread name mapping
    std::unordered_map<std::thread::id, std::string> threadNames_;
    
    uint64_t getTimestamp() const;
    uint32_t getTid() const;
    static uint32_t getPid();
    
    void addEvent(TraceEvent event);
    std::string eventToJSON(const TraceEvent& event) const;
};

// =============================================================================
// Trace Scope (RAII)
// =============================================================================

class TraceScope {
public:
    TraceScope(TracingProfiler& profiler, 
               const std::string& name,
               const std::string& category = TraceCategory::JS_EXECUTION)
        : profiler_(profiler), name_(name), category_(category) {
        profiler_.beginEvent(name_, category_);
    }
    
    ~TraceScope() {
        profiler_.endEvent(name_, category_);
    }
    
private:
    TracingProfiler& profiler_;
    std::string name_;
    std::string category_;
};

// =============================================================================
// Convenience Macros
// =============================================================================

#ifdef ZEPRA_ENABLE_TRACING

#define TRACE_EVENT_BEGIN(profiler, name, cat) (profiler).beginEvent(name, cat)
#define TRACE_EVENT_END(profiler, name, cat) (profiler).endEvent(name, cat)
#define TRACE_SCOPE(profiler, name) TraceScope _trace_scope_##__LINE__(profiler, name)
#define TRACE_INSTANT(profiler, name) (profiler).instantEvent(name)

#else

#define TRACE_EVENT_BEGIN(profiler, name, cat) ((void)0)
#define TRACE_EVENT_END(profiler, name, cat) ((void)0)
#define TRACE_SCOPE(profiler, name) ((void)0)
#define TRACE_INSTANT(profiler, name) ((void)0)

#endif

// =============================================================================
// Global Profiler Access
// =============================================================================

TracingProfiler& getGlobalTracer();

} // namespace Zepra::Perf
