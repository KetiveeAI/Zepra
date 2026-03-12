/**
 * @file task_manager_cpp.hpp
 * @brief C++ wrapper for ZepraBrowser Task Manager
 */

#pragma once

extern "C" {
#include "core/include/browser_core/task_manager.h"
}

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {

// =============================================================================
// Enums
// =============================================================================

enum class TaskType {
    Tab       = ZTM_TYPE_TAB,
    Extension = ZTM_TYPE_EXTENSION,
    Service   = ZTM_TYPE_SERVICE,
    Subframe  = ZTM_TYPE_SUBFRAME,
    GPU       = ZTM_TYPE_GPU,
    Network   = ZTM_TYPE_NETWORK,
    Audio     = ZTM_TYPE_AUDIO,
    Utility   = ZTM_TYPE_UTILITY,
};

enum class TaskState {
    Creating   = ZTM_STATE_CREATING,
    Running    = ZTM_STATE_RUNNING,
    Paused     = ZTM_STATE_PAUSED,
    Suspended  = ZTM_STATE_SUSPENDED,
    Terminated = ZTM_STATE_TERMINATED,
    Crashed    = ZTM_STATE_CRASHED,
};

enum class Priority {
    Lowest  = ZTM_PRIORITY_LOWEST,
    Low     = ZTM_PRIORITY_LOW,
    Normal  = ZTM_PRIORITY_NORMAL,
    High    = ZTM_PRIORITY_HIGH,
    Highest = ZTM_PRIORITY_HIGHEST,
};

// =============================================================================
// Structs
// =============================================================================

struct MemoryStats {
    uint64_t used;
    uint64_t limit;
    uint64_t peak;
    uint64_t jsHeapSize;
    uint64_t jsHeapUsed;
    uint64_t domNodes;
    
    double usedMB() const { return used / (1024.0 * 1024.0); }
    double limitMB() const { return limit / (1024.0 * 1024.0); }
    double usagePercent() const { return limit > 0 ? (double)used / limit * 100.0 : 0; }
};

struct CpuStats {
    double usage;
    double userTime;
    double systemTime;
    uint64_t contextSwitches;
    int threads;
};

struct NetworkStats {
    uint64_t bytesSent;
    uint64_t bytesReceived;
    int activeConnections;
    int pendingRequests;
};

struct TaskStats {
    MemoryStats memory;
    CpuStats cpu;
    NetworkStats network;
    
    double fps;
    double frameTimeAvg;
    double frameTimeMax;
    bool playingAudio;
    bool playingVideo;
    double uptime;
};

struct TaskInfo {
    int id;
    TaskType type;
    TaskState state;
    Priority priority;
    std::string title;
    std::string url;
    std::string favicon;
    int tabId;
    int processId;
    int parentTask;
    bool isolated;
    bool sandboxed;
    bool frozen;
    
    static TaskInfo fromC(const ztm_task_info_t& c) {
        TaskInfo info;
        info.id = c.id;
        info.type = static_cast<TaskType>(c.type);
        info.state = static_cast<TaskState>(c.state);
        info.priority = static_cast<Priority>(c.priority);
        info.title = c.title;
        info.url = c.url;
        info.favicon = c.favicon;
        info.tabId = c.tab_id;
        info.processId = c.process_id;
        info.parentTask = c.parent_task;
        info.isolated = c.isolated;
        info.sandboxed = c.sandboxed;
        info.frozen = c.frozen;
        return info;
    }
};

struct MemoryConfig {
    uint64_t memoryLimit = 0;
    uint64_t jsHeapLimit = 0;
    bool enableOomKill = false;
    bool enableThrottle = true;
    
    ztm_memory_config_t toC() const {
        ztm_memory_config_t c = {};
        c.memory_limit = memoryLimit;
        c.js_heap_limit = jsHeapLimit;
        c.enable_oom_kill = enableOomKill ? 1 : 0;
        c.enable_throttle = enableThrottle ? 1 : 0;
        return c;
    }
};

struct SystemStats {
    double cpuUsage;
    int cpuCores;
    uint64_t memoryTotal;
    uint64_t memoryUsed;
    uint64_t memoryBrowser;
    int processCount;
    int threadCount;
    uint64_t networkIn;
    uint64_t networkOut;
    double gpuUsage;
    uint64_t gpuMemoryUsed;
    uint64_t gpuMemoryTotal;
};

// =============================================================================
// Task class
// =============================================================================

class Task {
public:
    explicit Task(ztm_task_t handle) : handle_(handle) {}
    
    // Info
    TaskInfo info() const {
        ztm_task_info_t c;
        ztm_task_info(handle_, &c);
        return TaskInfo::fromC(c);
    }
    
    TaskStats stats() const {
        ztm_task_stats_t c;
        ztm_task_stats(handle_, &c);
        
        TaskStats s;
        s.memory.used = c.memory.memory_used;
        s.memory.limit = c.memory.memory_limit;
        s.memory.peak = c.memory.memory_peak;
        s.memory.jsHeapSize = c.memory.js_heap_size;
        s.memory.jsHeapUsed = c.memory.js_heap_used;
        s.memory.domNodes = c.memory.dom_nodes;
        
        s.cpu.usage = c.cpu.cpu_usage;
        s.cpu.userTime = c.cpu.cpu_time_user;
        s.cpu.systemTime = c.cpu.cpu_time_system;
        s.cpu.contextSwitches = c.cpu.context_switches;
        s.cpu.threads = c.cpu.threads;
        
        s.network.bytesSent = c.network.bytes_sent;
        s.network.bytesReceived = c.network.bytes_received;
        s.network.activeConnections = c.network.active_connections;
        s.network.pendingRequests = c.network.requests_pending;
        
        s.fps = c.fps;
        s.frameTimeAvg = c.frame_time_avg;
        s.frameTimeMax = c.frame_time_max;
        s.playingAudio = c.playing_audio;
        s.playingVideo = c.playing_video;
        s.uptime = c.uptime_seconds;
        
        return s;
    }
    
    // Control
    void end() { ztm_task_end(handle_); }
    void kill() { ztm_task_kill(handle_); }
    void pause() { ztm_task_pause(handle_); }
    void resume() { ztm_task_resume(handle_); }
    void suspend() { ztm_task_suspend(handle_); }
    
    void setPriority(Priority p) {
        ztm_task_set_priority(handle_, static_cast<ztm_priority_t>(p));
    }
    
    // Isolation
    void isolate() { ztm_task_isolate(handle_); }
    bool isIsolated() const { return ztm_task_is_isolated(handle_); }
    void setSandbox(bool enable) { ztm_task_sandbox(handle_, enable ? 1 : 0); }
    
    // Memory
    void setMemoryLimit(uint64_t bytes) { ztm_task_set_memory_limit(handle_, bytes); }
    void setMemoryLimitMB(uint64_t mb) { setMemoryLimit(mb * 1024 * 1024); }
    
    MemoryConfig memoryConfig() const {
        ztm_memory_config_t c;
        ztm_task_get_memory_config(handle_, &c);
        MemoryConfig config;
        config.memoryLimit = c.memory_limit;
        config.jsHeapLimit = c.js_heap_limit;
        config.enableOomKill = c.enable_oom_kill;
        config.enableThrottle = c.enable_throttle;
        return config;
    }
    
    void setMemoryConfig(const MemoryConfig& config) {
        auto c = config.toC();
        ztm_task_set_memory_config(handle_, &c);
    }
    
    void trimMemory() { ztm_task_trim_memory(handle_); }
    
    ztm_task_t handle() const { return handle_; }
    
private:
    ztm_task_t handle_;
};

// =============================================================================
// TaskManager (singleton)
// =============================================================================

class TaskManager {
public:
    using StatsCallback = std::function<void(Task&, const TaskStats&)>;
    using StateCallback = std::function<void(Task&, TaskState)>;
    using CrashCallback = std::function<void(Task&, int, const std::string&)>;
    
    static TaskManager& instance() {
        static TaskManager tm;
        return tm;
    }
    
    // Task list
    int taskCount() const { return ztm_task_count(); }
    
    std::vector<Task> tasks() const {
        std::vector<ztm_task_t> ids(256);
        int count = ztm_task_list(ids.data(), 256);
        
        std::vector<Task> result;
        result.reserve(count);
        for (int i = 0; i < count; i++) {
            result.emplace_back(ids[i]);
        }
        return result;
    }
    
    Task* findTask(int id) {
        auto t = tasks();
        for (auto& task : t) {
            if (task.handle() == id) {
                return &task;
            }
        }
        return nullptr;
    }
    
    // Create tasks
    Task createTabTask(int tabId, const std::string& url, const std::string& title) {
        ztm_task_t id = ztm_create_tab_task(tabId, url.c_str(), title.c_str());
        return Task(id);
    }
    
    Task createExtensionTask(const std::string& extId, const std::string& name) {
        ztm_task_t id = ztm_create_extension_task(extId.c_str(), name.c_str());
        return Task(id);
    }
    
    Task createServiceTask(const std::string& scope) {
        ztm_task_t id = ztm_create_service_task(scope.c_str());
        return Task(id);
    }
    
    void removeTask(Task& task) {
        ztm_remove_task(task.handle());
    }
    
    // Monitoring
    void startMonitor(int intervalMs = 1000) {
        ztm_monitor_start(intervalMs);
    }
    
    void stopMonitor() {
        ztm_monitor_stop();
    }
    
    void onStats(StatsCallback callback) {
        statsCallback_ = std::move(callback);
        ztm_on_stats([](void* ud, ztm_task_t task, const ztm_task_stats_t* stats) {
            auto* self = static_cast<TaskManager*>(ud);
            if (self->statsCallback_) {
                Task t(task);
                TaskStats s;
                // Convert stats...
                self->statsCallback_(t, s);
            }
        }, this);
    }
    
    void onStateChange(StateCallback callback) {
        stateCallback_ = std::move(callback);
        ztm_on_state_change([](void* ud, ztm_task_t task, ztm_task_state_t state) {
            auto* self = static_cast<TaskManager*>(ud);
            if (self->stateCallback_) {
                Task t(task);
                self->stateCallback_(t, static_cast<TaskState>(state));
            }
        }, this);
    }
    
    // System stats
    SystemStats systemStats() const {
        ztm_system_stats_t c;
        ztm_system_stats(&c);
        
        SystemStats s;
        s.cpuUsage = c.cpu_usage_total;
        s.cpuCores = c.cpu_cores;
        s.memoryTotal = c.memory_total;
        s.memoryUsed = c.memory_used;
        s.memoryBrowser = c.memory_browser;
        s.processCount = c.process_count;
        s.threadCount = c.thread_count;
        s.networkIn = c.network_bytes_in;
        s.networkOut = c.network_bytes_out;
        s.gpuUsage = c.gpu_usage;
        s.gpuMemoryUsed = c.gpu_memory_used;
        s.gpuMemoryTotal = c.gpu_memory_total;
        return s;
    }
    
    uint64_t totalMemoryUsage() const { return ztm_total_memory_usage(); }
    uint64_t systemMemoryAvailable() const { return ztm_system_memory_available(); }
    
private:
    TaskManager() { ztm_init(); }
    ~TaskManager() { ztm_shutdown(); }
    
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    
    StatsCallback statsCallback_;
    StateCallback stateCallback_;
    CrashCallback crashCallback_;
};

} // namespace zepra
