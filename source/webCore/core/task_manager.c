/*
 * ZepraBrowser Task Manager Implementation
 * 
 * Provides real-time process/tab monitoring and control.
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "task_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <signal.h>
#endif

/* ============ Internal State ============ */
#define MAX_TASKS 256

typedef struct {
    ztm_task_info_t     info;
    ztm_task_stats_t    stats;
    ztm_memory_config_t memory_config;
    
    /* Timing */
    struct timespec     created_at;
    struct timespec     last_update;
    
    /* Previous stats for delta calculation */
    uint64_t            prev_cpu_user;
    uint64_t            prev_cpu_system;
    uint64_t            prev_bytes_sent;
    uint64_t            prev_bytes_recv;
    
} task_entry_t;

static task_entry_t g_tasks[MAX_TASKS];
static int g_next_task_id = 1;
static int g_initialized = 0;

/* Monitoring */
static pthread_t g_monitor_thread;
static int g_monitor_running = 0;
static int g_monitor_interval_ms = 1000;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Callbacks */
static ztm_stats_cb g_stats_callback = NULL;
static void *g_stats_callback_data = NULL;
static ztm_state_cb g_state_callback = NULL;
static void *g_state_callback_data = NULL;
static ztm_crash_cb g_crash_callback = NULL;
static void *g_crash_callback_data = NULL;

/* ============ Internal Helpers ============ */

static task_entry_t* get_task(ztm_task_t id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id == id) {
            return &g_tasks[i];
        }
    }
    return NULL;
}

static double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void update_task_state(task_entry_t *task, ztm_task_state_t state) {
    if (task->info.state != state) {
        ztm_task_state_t old_state = task->info.state;
        task->info.state = state;
        
        if (g_state_callback) {
            g_state_callback(g_state_callback_data, task->info.id, state);
        }
        
        printf("[TaskManager] Task %d state: %d -> %d\n", 
               task->info.id, old_state, state);
    }
}

#ifdef __linux__
static int read_proc_stat(int pid, uint64_t *utime, uint64_t *stime, uint64_t *vsize, uint64_t *rss) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int flags;
    unsigned long minflt, cminflt, majflt, cmajflt;
    unsigned long ut, st;
    long cutime, cstime, priority, nice, num_threads;
    unsigned long itrealvalue, starttime, vs;
    long rs;
    
    int n = fscanf(f, "%*d %255s %c %d %d %d %d %d %u "
                   "%lu %lu %lu %lu %lu %lu "
                   "%ld %ld %ld %ld %ld %ld "
                   "%lu %lu %lu %ld",
                   comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                   &flags, &minflt, &cminflt, &majflt, &cmajflt, &ut, &st,
                   &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue,
                   &starttime, &vs, &rs);
    fclose(f);
    
    if (n < 24) return -1;
    
    if (utime) *utime = ut;
    if (stime) *stime = st;
    if (vsize) *vsize = vs;
    if (rss) *rss = rs * sysconf(_SC_PAGESIZE);
    
    return 0;
}

static int count_threads(int pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/task", pid);
    
    DIR *dir = opendir(path);
    if (!dir) return 1;
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') count++;
    }
    closedir(dir);
    
    return count;
}
#endif

static void update_task_stats(task_entry_t *task) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    double elapsed = (now.tv_sec - task->last_update.tv_sec) +
                     (now.tv_nsec - task->last_update.tv_nsec) / 1e9;
    
    if (elapsed < 0.1) return;  /* Min 100ms between updates */
    
    task->last_update = now;
    
    /* Calculate uptime */
    task->stats.uptime_seconds = (now.tv_sec - task->created_at.tv_sec) +
                                  (now.tv_nsec - task->created_at.tv_nsec) / 1e9;
    
    task->stats.timestamp = (uint64_t)time(NULL);
    
#ifdef __linux__
    int pid = task->info.process_id;
    if (pid > 0) {
        uint64_t utime, stime, vsize, rss;
        if (read_proc_stat(pid, &utime, &stime, &vsize, &rss) == 0) {
            /* CPU usage calculation */
            long ticks_per_sec = sysconf(_SC_CLK_TCK);
            double user_sec = (double)(utime - task->prev_cpu_user) / ticks_per_sec;
            double sys_sec = (double)(stime - task->prev_cpu_system) / ticks_per_sec;
            
            task->stats.cpu.cpu_usage = ((user_sec + sys_sec) / elapsed) * 100.0;
            task->stats.cpu.cpu_time_user = (double)utime / ticks_per_sec;
            task->stats.cpu.cpu_time_system = (double)stime / ticks_per_sec;
            
            task->prev_cpu_user = utime;
            task->prev_cpu_system = stime;
            
            /* Memory */
            task->stats.memory.memory_used = rss;
            if (rss > task->stats.memory.memory_peak) {
                task->stats.memory.memory_peak = rss;
            }
            
            /* Threads */
            task->stats.cpu.threads = count_threads(pid);
        }
    }
#else
    /* Simulate stats for non-Linux */
    task->stats.cpu.cpu_usage = 1.0 + (rand() % 50) / 10.0;
    task->stats.memory.memory_used = 10 * 1024 * 1024 + rand() % (100 * 1024 * 1024);
#endif
    
    /* Check memory limit */
    if (task->memory_config.memory_limit > 0) {
        task->stats.memory.memory_limit = task->memory_config.memory_limit;
        
        if (task->stats.memory.memory_used > task->memory_config.memory_limit) {
            if (task->memory_config.enable_oom_kill) {
                printf("[TaskManager] Task %d exceeded memory limit, killing\n", 
                       task->info.id);
                ztm_task_kill(task->info.id);
            } else if (task->memory_config.enable_throttle) {
                /* Throttle the task */
                if (task->info.state == ZTM_STATE_RUNNING) {
                    ztm_task_suspend(task->info.id);
                }
            }
        }
    }
}

static void* monitor_thread_func(void *arg) {
    (void)arg;
    
    printf("[TaskManager] Monitor thread started (interval=%dms)\n", 
           g_monitor_interval_ms);
    
    while (g_monitor_running) {
        pthread_mutex_lock(&g_mutex);
        
        for (int i = 0; i < MAX_TASKS; i++) {
            if (g_tasks[i].info.id != 0 && 
                g_tasks[i].info.state == ZTM_STATE_RUNNING) {
                update_task_stats(&g_tasks[i]);
                
                if (g_stats_callback) {
                    g_stats_callback(g_stats_callback_data, 
                                     g_tasks[i].info.id, 
                                     &g_tasks[i].stats);
                }
            }
        }
        
        pthread_mutex_unlock(&g_mutex);
        
        usleep(g_monitor_interval_ms * 1000);
    }
    
    printf("[TaskManager] Monitor thread stopped\n");
    return NULL;
}

/* ===========================================================================
 * Task Manager API
 * =========================================================================*/

int ztm_init(void) {
    if (g_initialized) return 0;
    
    printf("[TaskManager] Initializing\n");
    
    memset(g_tasks, 0, sizeof(g_tasks));
    g_initialized = 1;
    
    return 0;
}

void ztm_shutdown(void) {
    if (!g_initialized) return;
    
    ztm_monitor_stop();
    
    /* Kill all tasks */
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id != 0) {
            ztm_task_kill(g_tasks[i].info.id);
        }
    }
    
    g_initialized = 0;
    printf("[TaskManager] Shutdown\n");
}

int ztm_task_count(void) {
    int count = 0;
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id != 0) count++;
    }
    pthread_mutex_unlock(&g_mutex);
    return count;
}

int ztm_task_list(ztm_task_t *tasks, int max_tasks) {
    int count = 0;
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_TASKS && count < max_tasks; i++) {
        if (g_tasks[i].info.id != 0) {
            tasks[count++] = g_tasks[i].info.id;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return count;
}

int ztm_task_info(ztm_task_t task, ztm_task_info_t *info) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    *info = t->info;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_stats(ztm_task_t task, ztm_task_stats_t *stats) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    *stats = t->stats;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_all_stats(ztm_task_stats_t *stats, ztm_task_t *tasks, int max_tasks) {
    int count = 0;
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_TASKS && count < max_tasks; i++) {
        if (g_tasks[i].info.id != 0) {
            stats[count] = g_tasks[i].stats;
            tasks[count] = g_tasks[i].info.id;
            count++;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return count;
}

/* ===========================================================================
 * Task Control
 * =========================================================================*/

int ztm_task_end(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    printf("[TaskManager] Ending task %d (%s)\n", task, t->info.title);
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        kill(t->info.process_id, SIGTERM);
    }
#endif
    
    update_task_state(t, ZTM_STATE_TERMINATED);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_kill(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    printf("[TaskManager] Force killing task %d\n", task);
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        kill(t->info.process_id, SIGKILL);
    }
#endif
    
    update_task_state(t, ZTM_STATE_TERMINATED);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_pause(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        kill(t->info.process_id, SIGSTOP);
    }
#endif
    
    update_task_state(t, ZTM_STATE_PAUSED);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_resume(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        kill(t->info.process_id, SIGCONT);
    }
#endif
    
    update_task_state(t, ZTM_STATE_RUNNING);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_suspend(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    t->info.frozen = 1;
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        kill(t->info.process_id, SIGSTOP);
    }
#endif
    
    update_task_state(t, ZTM_STATE_SUSPENDED);
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_set_priority(ztm_task_t task, ztm_priority_t priority) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    t->info.priority = priority;
    
#ifdef __linux__
    if (t->info.process_id > 0) {
        /* Map priority to nice value */
        int nice_val = 10 - (priority * 5);  /* Lowest=10, Highest=-10 */
        setpriority(PRIO_PROCESS, t->info.process_id, nice_val);
    }
#endif
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

/* ===========================================================================
 * Process Isolation
 * =========================================================================*/

int ztm_task_isolate(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    t->info.isolated = 1;
    printf("[TaskManager] Task %d now isolated\n", task);
    
    /* In real implementation:
     * - Move to separate process
     * - Set up IPC
     * - Configure namespaces
     */
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_is_isolated(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    int isolated = t ? t->info.isolated : 0;
    pthread_mutex_unlock(&g_mutex);
    return isolated;
}

int ztm_task_sandbox(ztm_task_t task, int enable) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    t->info.sandboxed = enable ? 1 : 0;
    printf("[TaskManager] Task %d sandbox: %s\n", task, enable ? "enabled" : "disabled");
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

/* ===========================================================================
 * Memory Management
 * =========================================================================*/

int ztm_task_set_memory_limit(ztm_task_t task, uint64_t limit_bytes) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    t->memory_config.memory_limit = limit_bytes;
    t->stats.memory.memory_limit = limit_bytes;
    
    printf("[TaskManager] Task %d memory limit: %lu MB\n", 
           task, limit_bytes / (1024 * 1024));
    
#ifdef __linux__
    /* In real implementation: use cgroups to enforce limit */
#endif
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_get_memory_config(ztm_task_t task, ztm_memory_config_t *config) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    *config = t->memory_config;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_set_memory_config(ztm_task_t task, const ztm_memory_config_t *config) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t || !config) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    t->memory_config = *config;
    t->stats.memory.memory_limit = config->memory_limit;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

int ztm_task_trim_memory(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    
    printf("[TaskManager] Trimming memory for task %d\n", task);
    
    /* In real implementation:
     * - Trigger GC in JavaScript
     * - Clear caches
     * - Release unused memory
     */
    
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

uint64_t ztm_total_memory_usage(void) {
    uint64_t total = 0;
    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id != 0) {
            total += g_tasks[i].stats.memory.memory_used;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return total;
}

uint64_t ztm_system_memory_available(void) {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.freeram * si.mem_unit;
    }
#endif
    return 0;
}

/* ===========================================================================
 * Real-time Monitoring
 * =========================================================================*/

int ztm_monitor_start(int interval_ms) {
    if (g_monitor_running) return 0;
    
    g_monitor_interval_ms = interval_ms > 0 ? interval_ms : 1000;
    g_monitor_running = 1;
    
    pthread_create(&g_monitor_thread, NULL, monitor_thread_func, NULL);
    
    return 0;
}

int ztm_monitor_stop(void) {
    if (!g_monitor_running) return 0;
    
    g_monitor_running = 0;
    pthread_join(g_monitor_thread, NULL);
    
    return 0;
}

void ztm_on_stats(ztm_stats_cb callback, void *userdata) {
    g_stats_callback = callback;
    g_stats_callback_data = userdata;
}

void ztm_on_state_change(ztm_state_cb callback, void *userdata) {
    g_state_callback = callback;
    g_state_callback_data = userdata;
}

void ztm_on_crash(ztm_crash_cb callback, void *userdata) {
    g_crash_callback = callback;
    g_crash_callback_data = userdata;
}

/* ===========================================================================
 * System Stats
 * =========================================================================*/

int ztm_system_stats(ztm_system_stats_t *stats) {
    if (!stats) return -1;
    
    memset(stats, 0, sizeof(ztm_system_stats_t));
    
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        stats->memory_total = si.totalram * si.mem_unit;
        stats->memory_used = (si.totalram - si.freeram) * si.mem_unit;
        stats->process_count = si.procs;
    }
    
    stats->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    stats->memory_browser = ztm_total_memory_usage();
    
    /* CPU usage from /proc/stat */
    FILE *f = fopen("/proc/stat", "r");
    if (f) {
        unsigned long user, nice, system, idle;
        fscanf(f, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
        fclose(f);
        
        unsigned long total = user + nice + system + idle;
        unsigned long active = user + nice + system;
        stats->cpu_usage_total = (double)active / total * 100.0;
    }
#endif
    
    return 0;
}

/* ===========================================================================
 * Task Creation
 * =========================================================================*/

ztm_task_t ztm_create_tab_task(int32_t tab_id, const char *url, const char *title) {
    pthread_mutex_lock(&g_mutex);
    
    /* Find free slot */
    task_entry_t *t = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id == 0) {
            t = &g_tasks[i];
            break;
        }
    }
    
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return ZTM_INVALID_HANDLE;
    }
    
    memset(t, 0, sizeof(task_entry_t));
    t->info.id = g_next_task_id++;
    t->info.type = ZTM_TYPE_TAB;
    t->info.state = ZTM_STATE_RUNNING;
    t->info.priority = ZTM_PRIORITY_NORMAL;
    t->info.tab_id = tab_id;
    t->info.process_id = getpid();  /* In real impl: fork or separate process */
    
    if (url) strncpy(t->info.url, url, 1023);
    if (title) strncpy(t->info.title, title, 255);
    
    clock_gettime(CLOCK_MONOTONIC, &t->created_at);
    t->last_update = t->created_at;
    
    /* Default memory config */
    t->memory_config.memory_limit = 0;  /* Unlimited */
    t->memory_config.enable_oom_kill = 0;
    t->memory_config.enable_throttle = 1;
    
    printf("[TaskManager] Created tab task %d: %s\n", t->info.id, title ?: url);
    
    pthread_mutex_unlock(&g_mutex);
    return t->info.id;
}

ztm_task_t ztm_create_extension_task(const char *extension_id, const char *name) {
    pthread_mutex_lock(&g_mutex);
    
    task_entry_t *t = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id == 0) {
            t = &g_tasks[i];
            break;
        }
    }
    
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return ZTM_INVALID_HANDLE;
    }
    
    memset(t, 0, sizeof(task_entry_t));
    t->info.id = g_next_task_id++;
    t->info.type = ZTM_TYPE_EXTENSION;
    t->info.state = ZTM_STATE_RUNNING;
    t->info.priority = ZTM_PRIORITY_LOW;
    
    if (name) strncpy(t->info.title, name, 255);
    if (extension_id) strncpy(t->info.url, extension_id, 1023);
    
    clock_gettime(CLOCK_MONOTONIC, &t->created_at);
    t->last_update = t->created_at;
    
    pthread_mutex_unlock(&g_mutex);
    return t->info.id;
}

ztm_task_t ztm_create_service_task(const char *scope) {
    pthread_mutex_lock(&g_mutex);
    
    task_entry_t *t = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].info.id == 0) {
            t = &g_tasks[i];
            break;
        }
    }
    
    if (!t) {
        pthread_mutex_unlock(&g_mutex);
        return ZTM_INVALID_HANDLE;
    }
    
    memset(t, 0, sizeof(task_entry_t));
    t->info.id = g_next_task_id++;
    t->info.type = ZTM_TYPE_SERVICE;
    t->info.state = ZTM_STATE_RUNNING;
    t->info.priority = ZTM_PRIORITY_NORMAL;
    
    snprintf(t->info.title, 255, "Service Worker");
    if (scope) strncpy(t->info.url, scope, 1023);
    
    clock_gettime(CLOCK_MONOTONIC, &t->created_at);
    t->last_update = t->created_at;
    
    pthread_mutex_unlock(&g_mutex);
    return t->info.id;
}

void ztm_remove_task(ztm_task_t task) {
    pthread_mutex_lock(&g_mutex);
    task_entry_t *t = get_task(task);
    if (t) {
        printf("[TaskManager] Removed task %d\n", task);
        t->info.id = 0;
    }
    pthread_mutex_unlock(&g_mutex);
}
