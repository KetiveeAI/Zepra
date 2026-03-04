/*
 * ZepraBrowser Task Manager
 * 
 * Process/Tab management with:
 * - Per-tab process isolation
 * - Memory allocation limits
 * - Real-time performance monitoring
 * - Task termination
 * 
 * Copyright (c) 2025 KetiveeAI - KETIVEEAI License
 */

#ifndef ZEPRA_TASK_MANAGER_H
#define ZEPRA_TASK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ============ Handle Types ============ */
typedef int32_t ztm_task_t;      /* Task/Tab handle */
typedef int32_t ztm_process_t;  /* Process handle */

#define ZTM_INVALID_HANDLE (-1)

/* ============ Task Types ============ */
typedef enum {
    ZTM_TYPE_TAB           = 0,  /* Browser tab */
    ZTM_TYPE_EXTENSION     = 1,  /* Browser extension */
    ZTM_TYPE_SERVICE       = 2,  /* Service worker */
    ZTM_TYPE_SUBFRAME      = 3,  /* Iframe */
    ZTM_TYPE_GPU           = 4,  /* GPU process */
    ZTM_TYPE_NETWORK       = 5,  /* Network service */
    ZTM_TYPE_AUDIO         = 6,  /* Audio process */
    ZTM_TYPE_UTILITY       = 7,  /* Utility process */
} ztm_task_type_t;

/* ============ Task State ============ */
typedef enum {
    ZTM_STATE_CREATING     = 0,
    ZTM_STATE_RUNNING      = 1,
    ZTM_STATE_PAUSED       = 2,
    ZTM_STATE_SUSPENDED    = 3,
    ZTM_STATE_TERMINATED   = 4,
    ZTM_STATE_CRASHED      = 5,
} ztm_task_state_t;

/* ============ Priority Levels ============ */
typedef enum {
    ZTM_PRIORITY_LOWEST    = 0,
    ZTM_PRIORITY_LOW       = 1,
    ZTM_PRIORITY_NORMAL    = 2,
    ZTM_PRIORITY_HIGH      = 3,
    ZTM_PRIORITY_HIGHEST   = 4,
} ztm_priority_t;

/* ============ Memory Stats ============ */
typedef struct {
    uint64_t    memory_used;        /* Current memory usage (bytes) */
    uint64_t    memory_limit;       /* Memory limit (0 = unlimited) */
    uint64_t    memory_peak;        /* Peak memory usage */
    uint64_t    js_heap_size;       /* JavaScript heap size */
    uint64_t    js_heap_used;       /* JavaScript heap used */
    uint64_t    dom_nodes;          /* Number of DOM nodes */
    uint64_t    images_cached;      /* Cached images size */
} ztm_memory_stats_t;

/* ============ CPU Stats ============ */
typedef struct {
    double      cpu_usage;          /* CPU usage (0.0 - 100.0) */
    double      cpu_time_user;      /* User CPU time (seconds) */
    double      cpu_time_system;    /* System CPU time (seconds) */
    uint64_t    context_switches;   /* Context switches */
    int32_t     threads;            /* Number of threads */
} ztm_cpu_stats_t;

/* ============ Network Stats ============ */
typedef struct {
    uint64_t    bytes_sent;         /* Bytes sent */
    uint64_t    bytes_received;     /* Bytes received */
    int32_t     active_connections; /* Active connections */
    int32_t     requests_pending;   /* Pending requests */
} ztm_network_stats_t;

/* ============ Complete Task Stats ============ */
typedef struct {
    ztm_memory_stats_t   memory;
    ztm_cpu_stats_t      cpu;
    ztm_network_stats_t  network;
    
    /* Frame stats (for tabs) */
    double      fps;                /* Frames per second */
    double      frame_time_avg;     /* Average frame time (ms) */
    double      frame_time_max;     /* Max frame time (ms) */
    
    /* Media */
    uint8_t     playing_audio;
    uint8_t     playing_video;
    
    /* Timing */
    double      uptime_seconds;     /* Time since creation */
    uint64_t    timestamp;          /* Stats timestamp */
} ztm_task_stats_t;

/* ============ Task Info ============ */
typedef struct {
    ztm_task_t          id;
    ztm_task_type_t     type;
    ztm_task_state_t    state;
    ztm_priority_t      priority;
    
    char                title[256];
    char                url[1024];
    char                favicon[256];
    
    int32_t             tab_id;         /* Browser tab ID */
    int32_t             process_id;     /* OS process ID */
    int32_t             parent_task;    /* Parent task (for subframes) */
    
    uint8_t             isolated;       /* Process isolation enabled */
    uint8_t             sandboxed;      /* Sandbox enabled */
    uint8_t             frozen;         /* Tab frozen (suspended) */
} ztm_task_info_t;

/* ============ Memory Limits ============ */
typedef struct {
    uint64_t    memory_limit;       /* Max memory (bytes, 0 = unlimited) */
    uint64_t    js_heap_limit;      /* Max JS heap size */
    uint8_t     enable_oom_kill;    /* Kill on OOM */
    uint8_t     enable_throttle;    /* Throttle on high memory */
} ztm_memory_config_t;

/* ============ Callbacks ============ */
typedef void (*ztm_stats_cb)(void *userdata, ztm_task_t task, const ztm_task_stats_t *stats);
typedef void (*ztm_state_cb)(void *userdata, ztm_task_t task, ztm_task_state_t state);
typedef void (*ztm_crash_cb)(void *userdata, ztm_task_t task, int exit_code, const char *reason);

/* ===========================================================================
 * Task Manager API
 * =========================================================================*/

/**
 * Initialize task manager
 */
int ztm_init(void);

/**
 * Shutdown task manager
 */
void ztm_shutdown(void);

/**
 * Get number of tasks
 */
int ztm_task_count(void);

/**
 * Get all task IDs
 */
int ztm_task_list(ztm_task_t *tasks, int max_tasks);

/**
 * Get task info
 */
int ztm_task_info(ztm_task_t task, ztm_task_info_t *info);

/**
 * Get task stats
 */
int ztm_task_stats(ztm_task_t task, ztm_task_stats_t *stats);

/**
 * Get stats for all tasks
 */
int ztm_all_stats(ztm_task_stats_t *stats, ztm_task_t *tasks, int max_tasks);

/* ===========================================================================
 * Task Control
 * =========================================================================*/

/**
 * End/kill task
 */
int ztm_task_end(ztm_task_t task);

/**
 * Force kill task (SIGKILL)
 */
int ztm_task_kill(ztm_task_t task);

/**
 * Pause task
 */
int ztm_task_pause(ztm_task_t task);

/**
 * Resume task
 */
int ztm_task_resume(ztm_task_t task);

/**
 * Suspend/freeze task (stops execution, keeps memory)
 */
int ztm_task_suspend(ztm_task_t task);

/**
 * Set task priority
 */
int ztm_task_set_priority(ztm_task_t task, ztm_priority_t priority);

/* ===========================================================================
 * Process Isolation
 * =========================================================================*/

/**
 * Enable process isolation for task
 */
int ztm_task_isolate(ztm_task_t task);

/**
 * Check if task is isolated
 */
int ztm_task_is_isolated(ztm_task_t task);

/**
 * Enable sandboxing for task
 */
int ztm_task_sandbox(ztm_task_t task, int enable);

/* ===========================================================================
 * Memory Management
 * =========================================================================*/

/**
 * Set memory limit for task
 */
int ztm_task_set_memory_limit(ztm_task_t task, uint64_t limit_bytes);

/**
 * Get memory config for task
 */
int ztm_task_get_memory_config(ztm_task_t task, ztm_memory_config_t *config);

/**
 * Set memory config for task
 */
int ztm_task_set_memory_config(ztm_task_t task, const ztm_memory_config_t *config);

/**
 * Trim memory for task (free unused memory)
 */
int ztm_task_trim_memory(ztm_task_t task);

/**
 * Get total memory usage
 */
uint64_t ztm_total_memory_usage(void);

/**
 * Get system memory available
 */
uint64_t ztm_system_memory_available(void);

/* ===========================================================================
 * Real-time Monitoring
 * =========================================================================*/

/**
 * Start real-time stats collection
 */
int ztm_monitor_start(int interval_ms);

/**
 * Stop real-time stats collection
 */
int ztm_monitor_stop(void);

/**
 * Set stats callback (called on interval)
 */
void ztm_on_stats(ztm_stats_cb callback, void *userdata);

/**
 * Set state change callback
 */
void ztm_on_state_change(ztm_state_cb callback, void *userdata);

/**
 * Set crash callback
 */
void ztm_on_crash(ztm_crash_cb callback, void *userdata);

/* ===========================================================================
 * System Stats
 * =========================================================================*/

typedef struct {
    /* CPU */
    double      cpu_usage_total;
    int32_t     cpu_cores;
    
    /* Memory */
    uint64_t    memory_total;
    uint64_t    memory_used;
    uint64_t    memory_browser;     /* Memory used by browser */
    
    /* Processes */
    int32_t     process_count;
    int32_t     thread_count;
    
    /* Network */
    uint64_t    network_bytes_in;
    uint64_t    network_bytes_out;
    
    /* GPU */
    double      gpu_usage;
    uint64_t    gpu_memory_used;
    uint64_t    gpu_memory_total;
} ztm_system_stats_t;

/**
 * Get system-wide stats
 */
int ztm_system_stats(ztm_system_stats_t *stats);

/* ===========================================================================
 * Task Creation (for internal use)
 * =========================================================================*/

/**
 * Create task for tab
 */
ztm_task_t ztm_create_tab_task(int32_t tab_id, const char *url, const char *title);

/**
 * Create task for extension
 */
ztm_task_t ztm_create_extension_task(const char *extension_id, const char *name);

/**
 * Create task for service worker
 */
ztm_task_t ztm_create_service_task(const char *scope);

/**
 * Remove task
 */
void ztm_remove_task(ztm_task_t task);

#ifdef __cplusplus
}
#endif

#endif /* ZEPRA_TASK_MANAGER_H */
