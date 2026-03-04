/*
 * ZepraBrowser Task Auto-Manager Implementation
 * 
 * Automatic memory and process management.
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "browser_core/auto_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

/* ============ Internal State ============ */
static zam_config_t g_config;
static int g_enabled = 0;
static int g_initialized = 0;

static pthread_t g_auto_thread;
static int g_running = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Per-task state */
#define MAX_TASK_STATE 256
typedef struct {
    ztm_task_t  task;
    int         pinned;
    int         unsaved;
    uint64_t    last_active_time;
    zam_importance_t importance;
} task_state_t;

static task_state_t g_task_states[MAX_TASK_STATE];

/* ============ Internal Helpers ============ */

static task_state_t* get_task_state(ztm_task_t task) {
    for (int i = 0; i < MAX_TASK_STATE; i++) {
        if (g_task_states[i].task == task) {
            return &g_task_states[i];
        }
    }
    /* Create new */
    for (int i = 0; i < MAX_TASK_STATE; i++) {
        if (g_task_states[i].task == 0) {
            g_task_states[i].task = task;
            g_task_states[i].importance = ZAM_IMPORTANCE_NORMAL;
            g_task_states[i].last_active_time = (uint64_t)time(NULL);
            return &g_task_states[i];
        }
    }
    return NULL;
}

static uint64_t get_system_memory(void) {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.totalram * si.mem_unit;
    }
#endif
    return 8ULL * 1024 * 1024 * 1024;  /* Default 8GB */
}

static uint64_t get_available_memory(void) {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.freeram * si.mem_unit;
    }
#endif
    return 4ULL * 1024 * 1024 * 1024;  /* Default 4GB */
}

static void calculate_importance(task_state_t *ts) {
    ztm_task_stats_t stats;
    ztm_task_info_t info;
    
    if (ztm_task_stats(ts->task, &stats) != 0 ||
        ztm_task_info(ts->task, &info) != 0) {
        return;
    }
    
    /* Start with normal importance */
    zam_importance_t imp = ZAM_IMPORTANCE_NORMAL;
    
    /* Critical if pinned or has unsaved data */
    if (ts->pinned || ts->unsaved) {
        imp = ZAM_IMPORTANCE_CRITICAL;
    }
    /* High if playing media */
    else if (stats.playing_audio || stats.playing_video) {
        imp = ZAM_IMPORTANCE_HIGH;
    }
    /* Check last active time */
    else {
        uint64_t now = (uint64_t)time(NULL);
        uint64_t idle_time = now - ts->last_active_time;
        
        if (idle_time < 60) {
            imp = ZAM_IMPORTANCE_HIGH;  /* Active within 1 minute */
        } else if (idle_time < 300) {
            imp = ZAM_IMPORTANCE_NORMAL;  /* Active within 5 minutes */
        } else if (idle_time < 1800) {
            imp = ZAM_IMPORTANCE_LOW;  /* Active within 30 minutes */
        } else {
            imp = ZAM_IMPORTANCE_DISCARDABLE;  /* Idle > 30 minutes */
        }
    }
    
    ts->importance = imp;
}

static void perform_auto_management(void) {
    pthread_mutex_lock(&g_mutex);
    
    if (!g_enabled) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    
    double pressure = zam_memory_pressure();
    printf("[AutoManager] Memory pressure: %.1f%%\n", pressure * 100);
    
    /* Get all tasks */
    ztm_task_t tasks[256];
    ztm_task_stats_t stats[256];
    int count = ztm_all_stats(stats, tasks, 256);
    
    /* Calculate importance for all tasks */
    for (int i = 0; i < count; i++) {
        task_state_t *ts = get_task_state(tasks[i]);
        if (ts) {
            calculate_importance(ts);
        }
    }
    
    /* Critical memory pressure - kill discardable tabs */
    if (pressure > 0.9) {
        printf("[AutoManager] CRITICAL memory pressure, killing discardable tabs\n");
        for (int i = 0; i < count; i++) {
            task_state_t *ts = get_task_state(tasks[i]);
            if (ts && ts->importance == ZAM_IMPORTANCE_DISCARDABLE) {
                printf("[AutoManager] Killing discardable task %d\n", tasks[i]);
                ztm_task_kill(tasks[i]);
            }
        }
    }
    /* High pressure - suspend low importance tabs */
    else if (pressure > 0.75) {
        printf("[AutoManager] High memory pressure, suspending low-importance tabs\n");
        for (int i = 0; i < count; i++) {
            task_state_t *ts = get_task_state(tasks[i]);
            if (ts && ts->importance <= ZAM_IMPORTANCE_LOW) {
                ztm_task_info_t info;
                if (ztm_task_info(tasks[i], &info) == 0 && 
                    info.state == ZTM_STATE_RUNNING) {
                    printf("[AutoManager] Suspending task %d\n", tasks[i]);
                    ztm_task_suspend(tasks[i]);
                }
            }
        }
    }
    
    /* Auto-suspend old background tabs */
    if (g_config.auto_suspend_enabled) {
        uint64_t now = (uint64_t)time(NULL);
        for (int i = 0; i < count; i++) {
            task_state_t *ts = get_task_state(tasks[i]);
            if (!ts) continue;
            
            uint64_t idle_time = now - ts->last_active_time;
            if (idle_time > (uint64_t)g_config.auto_suspend_delay_sec &&
                ts->importance < ZAM_IMPORTANCE_HIGH) {
                
                ztm_task_info_t info;
                if (ztm_task_info(tasks[i], &info) == 0 && 
                    info.state == ZTM_STATE_RUNNING) {
                    printf("[AutoManager] Auto-suspending idle task %d (idle %lus)\n", 
                           tasks[i], (unsigned long)idle_time);
                    ztm_task_suspend(tasks[i]);
                }
            }
        }
    }
    
    /* Check per-tab memory limits */
    if (g_config.per_tab_memory_hard_limit > 0) {
        for (int i = 0; i < count; i++) {
            if (stats[i].memory.memory_used > g_config.per_tab_memory_hard_limit) {
                task_state_t *ts = get_task_state(tasks[i]);
                if (ts && ts->importance < ZAM_IMPORTANCE_CRITICAL) {
                    printf("[AutoManager] Task %d exceeded memory limit, killing\n", tasks[i]);
                    ztm_task_kill(tasks[i]);
                }
            }
        }
    }
    
    pthread_mutex_unlock(&g_mutex);
}

static void* auto_manager_thread(void *arg) {
    (void)arg;
    
    printf("[AutoManager] Thread started\n");
    
    while (g_running) {
        perform_auto_management();
        sleep(5);  /* Check every 5 seconds */
    }
    
    printf("[AutoManager] Thread stopped\n");
    return NULL;
}

/* ===========================================================================
 * Auto Manager API
 * =========================================================================*/

int zam_init(const zam_config_t *config) {
    if (g_initialized) return 0;
    
    printf("[AutoManager] Initializing\n");
    
    /* Default config */
    g_config.memory_warning_threshold = 1024ULL * 1024 * 1024;     /* 1 GB */
    g_config.memory_critical_threshold = 512ULL * 1024 * 1024;     /* 512 MB */
    g_config.per_tab_memory_soft_limit = 256ULL * 1024 * 1024;     /* 256 MB */
    g_config.per_tab_memory_hard_limit = 512ULL * 1024 * 1024;     /* 512 MB */
    g_config.auto_suspend_enabled = 1;
    g_config.auto_suspend_delay_sec = 300;  /* 5 minutes */
    g_config.auto_discard_enabled = 0;
    g_config.max_active_tabs = 10;
    g_config.cpu_throttle_threshold = 80.0;
    g_config.cpu_throttle_enabled = 1;
    g_config.prioritize_active_tab = 1;
    g_config.prioritize_audio_tabs = 1;
    g_config.prioritize_video_tabs = 1;
    
    if (config) {
        g_config = *config;
    }
    
    memset(g_task_states, 0, sizeof(g_task_states));
    
    /* Start auto manager thread */
    g_running = 1;
    pthread_create(&g_auto_thread, NULL, auto_manager_thread, NULL);
    
    g_initialized = 1;
    g_enabled = 1;
    
    return 0;
}

void zam_shutdown(void) {
    if (!g_initialized) return;
    
    g_running = 0;
    pthread_join(g_auto_thread, NULL);
    
    g_initialized = 0;
    printf("[AutoManager] Shutdown\n");
}

void zam_get_config(zam_config_t *config) {
    if (config) *config = g_config;
}

void zam_set_config(const zam_config_t *config) {
    if (config) {
        pthread_mutex_lock(&g_mutex);
        g_config = *config;
        pthread_mutex_unlock(&g_mutex);
    }
}

void zam_enable(int enabled) {
    g_enabled = enabled ? 1 : 0;
    printf("[AutoManager] %s\n", enabled ? "Enabled" : "Disabled");
}

int zam_is_enabled(void) {
    return g_enabled;
}

double zam_memory_pressure(void) {
    uint64_t available = get_available_memory();
    uint64_t total = get_system_memory();
    
    if (total == 0) return 0.0;
    
    double used_ratio = 1.0 - ((double)available / total);
    return used_ratio;
}

zam_importance_t zam_get_importance(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_state_t *ts = get_task_state(task);
    zam_importance_t imp = ts ? ts->importance : ZAM_IMPORTANCE_NORMAL;
    pthread_mutex_unlock(&g_mutex);
    return imp;
}

void zam_set_pinned(ztm_task_t task, int pinned) {
    pthread_mutex_lock(&g_mutex);
    task_state_t *ts = get_task_state(task);
    if (ts) {
        ts->pinned = pinned ? 1 : 0;
        if (pinned) ts->importance = ZAM_IMPORTANCE_CRITICAL;
    }
    pthread_mutex_unlock(&g_mutex);
}

void zam_set_unsaved(ztm_task_t task, int unsaved) {
    pthread_mutex_lock(&g_mutex);
    task_state_t *ts = get_task_state(task);
    if (ts) {
        ts->unsaved = unsaved ? 1 : 0;
        if (unsaved) ts->importance = ZAM_IMPORTANCE_CRITICAL;
    }
    pthread_mutex_unlock(&g_mutex);
}

void zam_optimize_now(void) {
    perform_auto_management();
}

zam_action_t zam_get_recommendation(ztm_task_t task) {
    ztm_task_stats_t stats;
    if (ztm_task_stats(task, &stats) != 0) {
        return ZAM_ACTION_NONE;
    }
    
    task_state_t *ts = get_task_state(task);
    if (!ts) return ZAM_ACTION_NONE;
    
    /* Critical tasks - no action */
    if (ts->importance >= ZAM_IMPORTANCE_CRITICAL) {
        return ZAM_ACTION_NONE;
    }
    
    double pressure = zam_memory_pressure();
    
    /* Memory exceeded hard limit */
    if (g_config.per_tab_memory_hard_limit > 0 &&
        stats.memory.memory_used > g_config.per_tab_memory_hard_limit) {
        return ZAM_ACTION_KILL;
    }
    
    /* Critical pressure */
    if (pressure > 0.9 && ts->importance <= ZAM_IMPORTANCE_DISCARDABLE) {
        return ZAM_ACTION_DISCARD;
    }
    
    /* High pressure */
    if (pressure > 0.75 && ts->importance <= ZAM_IMPORTANCE_LOW) {
        return ZAM_ACTION_SUSPEND;
    }
    
    /* Moderate pressure */
    if (pressure > 0.6) {
        return ZAM_ACTION_THROTTLE;
    }
    
    return ZAM_ACTION_NONE;
}

/* Mark tab as active (called when tab is focused) */
void zam_mark_active(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_state_t *ts = get_task_state(task);
    if (ts) {
        ts->last_active_time = (uint64_t)time(NULL);
    }
    pthread_mutex_unlock(&g_mutex);
}
