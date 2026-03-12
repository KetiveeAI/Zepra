/*
 * ZepraBrowser Task Manager UI Implementation
 */

#define _DEFAULT_SOURCE
#include "task_manager_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============ Internal State ============ */
static int g_advanced_unlocked = 0;
static const char* g_advanced_password = "zepra_admin";  /* Default password */

/* ============ Action Names ============ */
static const char* g_action_names[] = {
    "End task",
    "Isolate process",
    "Pin tab",
    "Force kill",
    "Pause",
    "Resume",
    "Suspend",
    "Set priority",
    "Set memory limit",
    "Trim memory",
    "Enable sandbox",
};

/* ============ Format Helpers ============ */

static void format_bytes(uint64_t bytes, char *out, size_t size) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        snprintf(out, size, "%.1f GB", bytes / (1024.0 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        snprintf(out, size, "%.1f MB", bytes / (1024.0 * 1024));
    } else if (bytes >= 1024) {
        snprintf(out, size, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(out, size, "%lu B", (unsigned long)bytes);
    }
}

static void format_network(uint64_t in, uint64_t out_bytes, char *result, size_t size) {
    char in_str[16], out_str[16];
    format_bytes(in, in_str, sizeof(in_str));
    format_bytes(out_bytes, out_str, sizeof(out_str));
    snprintf(result, size, "%s ↓ %s ↑", in_str, out_str);
}

static const char* task_type_str(ztm_task_type_t type) {
    switch (type) {
        case ZTM_TYPE_TAB: return "Tab";
        case ZTM_TYPE_EXTENSION: return "Extension";
        case ZTM_TYPE_SERVICE: return "Service Worker";
        case ZTM_TYPE_SUBFRAME: return "Iframe";
        case ZTM_TYPE_GPU: return "GPU Process";
        case ZTM_TYPE_NETWORK: return "Network";
        case ZTM_TYPE_AUDIO: return "Audio";
        case ZTM_TYPE_UTILITY: return "Utility";
        default: return "Unknown";
    }
}

static const char* task_state_str(ztm_task_state_t state) {
    switch (state) {
        case ZTM_STATE_CREATING: return "Creating";
        case ZTM_STATE_RUNNING: return "Running";
        case ZTM_STATE_PAUSED: return "Paused";
        case ZTM_STATE_SUSPENDED: return "Suspended";
        case ZTM_STATE_TERMINATED: return "Ended";
        case ZTM_STATE_CRASHED: return "Crashed";
        default: return "Unknown";
    }
}

static const char* priority_str(ztm_priority_t priority) {
    switch (priority) {
        case ZTM_PRIORITY_LOWEST: return "Lowest";
        case ZTM_PRIORITY_LOW: return "Low";
        case ZTM_PRIORITY_NORMAL: return "Normal";
        case ZTM_PRIORITY_HIGH: return "High";
        case ZTM_PRIORITY_HIGHEST: return "Highest";
        default: return "Normal";
    }
}

/* ===========================================================================
 * UI API
 * =========================================================================*/

void ztm_ui_init(ztm_ui_state_t *state) {
    if (!state) return;
    
    state->mode = ZTM_UI_MODE_NORMAL;
    state->sort_by = ZTM_SORT_MEMORY;
    state->sort_descending = 1;
    state->selected_task = ZTM_INVALID_HANDLE;
    state->refresh_interval_ms = 1000;
    state->auto_refresh = 1;
    
    /* Normal mode columns */
    state->columns.show_title = 1;
    state->columns.show_type = 1;
    state->columns.show_cpu = 1;
    state->columns.show_memory = 1;
    state->columns.show_network = 0;
    state->columns.show_process_id = 0;
    state->columns.show_state = 1;
    state->columns.show_priority = 0;
    state->columns.show_memory_limit = 0;
    state->columns.show_isolation = 0;
}

void ztm_ui_set_mode(ztm_ui_state_t *state, ztm_ui_mode_t mode) {
    if (!state) return;
    
    state->mode = mode;
    
    if (mode == ZTM_UI_MODE_ADVANCED) {
        /* Show all columns in advanced mode */
        state->columns.show_network = 1;
        state->columns.show_process_id = 1;
        state->columns.show_priority = 1;
        state->columns.show_memory_limit = 1;
        state->columns.show_isolation = 1;
    } else {
        /* Hide advanced columns in normal mode */
        state->columns.show_network = 0;
        state->columns.show_process_id = 0;
        state->columns.show_priority = 0;
        state->columns.show_memory_limit = 0;
        state->columns.show_isolation = 0;
    }
}

int ztm_ui_needs_advanced_unlock(void) {
    return !g_advanced_unlocked;
}

int ztm_ui_unlock_advanced(const char *password) {
    if (password && strcmp(password, g_advanced_password) == 0) {
        g_advanced_unlocked = 1;
        return 1;
    }
    return 0;
}

int ztm_ui_get_rows(const ztm_ui_state_t *state, ztm_ui_row_t *rows, int max_rows) {
    (void)state;
    
    ztm_task_t tasks[256];
    int count = ztm_task_list(tasks, max_rows < 256 ? max_rows : 256);
    
    for (int i = 0; i < count && i < max_rows; i++) {
        ztm_ui_row_t *row = &rows[i];
        memset(row, 0, sizeof(ztm_ui_row_t));
        
        row->id = tasks[i];
        
        ztm_task_info_t info;
        ztm_task_stats_t stats;
        
        if (ztm_task_info(tasks[i], &info) != 0) continue;
        if (ztm_task_stats(tasks[i], &stats) != 0) continue;
        
        /* Title and type */
        strncpy(row->title, info.title[0] ? info.title : info.url, 255);
        strncpy(row->type_str, task_type_str(info.type), 31);
        strncpy(row->state_str, task_state_str(info.state), 31);
        strncpy(row->priority_str, priority_str(info.priority), 15);
        
        /* CPU */
        row->cpu_usage = stats.cpu.cpu_usage;
        snprintf(row->cpu_str, sizeof(row->cpu_str), "%.1f%%", stats.cpu.cpu_usage);
        row->cpu_warning = stats.cpu.cpu_usage > 50;
        row->cpu_critical = stats.cpu.cpu_usage > 90;
        
        /* Memory */
        row->memory_used = stats.memory.memory_used;
        row->memory_limit = stats.memory.memory_limit;
        
        char used_str[16];
        format_bytes(stats.memory.memory_used, used_str, sizeof(used_str));
        
        if (stats.memory.memory_limit > 0) {
            char limit_str[16];
            format_bytes(stats.memory.memory_limit, limit_str, sizeof(limit_str));
            snprintf(row->memory_str, sizeof(row->memory_str), 
                     "%s / %s", used_str, limit_str);
            snprintf(row->memory_limit_str, sizeof(row->memory_limit_str), 
                     "%s", limit_str);
            
            double ratio = (double)stats.memory.memory_used / stats.memory.memory_limit;
            row->memory_warning = ratio > 0.75;
            row->memory_critical = ratio > 0.90;
        } else {
            snprintf(row->memory_str, sizeof(row->memory_str), "%s", used_str);
            snprintf(row->memory_limit_str, sizeof(row->memory_limit_str), "Auto");
        }
        
        /* Network */
        row->network_total = stats.network.bytes_sent + stats.network.bytes_received;
        format_network(stats.network.bytes_received, stats.network.bytes_sent,
                       row->network_str, sizeof(row->network_str));
        
        /* Flags */
        row->is_playing_audio = stats.playing_audio;
        row->is_playing_video = stats.playing_video;
        row->is_isolated = info.isolated;
        row->is_sandboxed = info.sandboxed;
        row->is_frozen = info.frozen;
    }
    
    return count;
}

int ztm_ui_perform_action(ztm_ui_action_t action, ztm_task_t task, void *param) {
    switch (action) {
        case ZTM_ACTION_END_TASK:
            return ztm_task_end(task);
            
        case ZTM_ACTION_ISOLATE:
            return ztm_task_isolate(task);
            
        case ZTM_ACTION_PIN:
            zam_set_pinned(task, 1);
            return 0;
            
        case ZTM_ACTION_KILL_FORCE:
            return ztm_task_kill(task);
            
        case ZTM_ACTION_PAUSE:
            return ztm_task_pause(task);
            
        case ZTM_ACTION_RESUME:
            return ztm_task_resume(task);
            
        case ZTM_ACTION_SUSPEND:
            return ztm_task_suspend(task);
            
        case ZTM_ACTION_SET_PRIORITY:
            if (param) {
                return ztm_task_set_priority(task, *(ztm_priority_t*)param);
            }
            return -1;
            
        case ZTM_ACTION_SET_MEMORY_LIMIT:
            if (param) {
                return ztm_task_set_memory_limit(task, *(uint64_t*)param);
            }
            return -1;
            
        case ZTM_ACTION_TRIM_MEMORY:
            return ztm_task_trim_memory(task);
            
        case ZTM_ACTION_SANDBOX:
            return ztm_task_sandbox(task, 1);
            
        default:
            return -1;
    }
}

int ztm_ui_get_available_actions(ztm_ui_mode_t mode, ztm_ui_action_t *actions, int max) {
    int count = 0;
    
    /* Normal mode actions */
    if (count < max) actions[count++] = ZTM_ACTION_END_TASK;
    if (count < max) actions[count++] = ZTM_ACTION_ISOLATE;
    if (count < max) actions[count++] = ZTM_ACTION_PIN;
    
    /* Advanced mode actions */
    if (mode == ZTM_UI_MODE_ADVANCED) {
        if (count < max) actions[count++] = ZTM_ACTION_KILL_FORCE;
        if (count < max) actions[count++] = ZTM_ACTION_PAUSE;
        if (count < max) actions[count++] = ZTM_ACTION_RESUME;
        if (count < max) actions[count++] = ZTM_ACTION_SUSPEND;
        if (count < max) actions[count++] = ZTM_ACTION_SET_PRIORITY;
        if (count < max) actions[count++] = ZTM_ACTION_SET_MEMORY_LIMIT;
        if (count < max) actions[count++] = ZTM_ACTION_TRIM_MEMORY;
        if (count < max) actions[count++] = ZTM_ACTION_SANDBOX;
    }
    
    return count;
}

const char* ztm_ui_action_name(ztm_ui_action_t action) {
    if (action >= 0 && action < (int)(sizeof(g_action_names) / sizeof(g_action_names[0]))) {
        return g_action_names[action];
    }
    return "Unknown";
}

int ztm_ui_action_available(ztm_ui_mode_t mode, ztm_ui_action_t action) {
    /* Normal mode: only end, isolate, pin */
    if (mode == ZTM_UI_MODE_NORMAL) {
        return action == ZTM_ACTION_END_TASK || 
               action == ZTM_ACTION_ISOLATE ||
               action == ZTM_ACTION_PIN;
    }
    
    /* Advanced mode: all actions */
    return 1;
}

/* ===========================================================================
 * Memory UI
 * =========================================================================*/

int ztm_ui_get_memory_info(ztm_task_t task, ztm_memory_ui_t *info) {
    if (!info) return -1;
    
    ztm_task_stats_t stats;
    ztm_memory_config_t config;
    
    if (ztm_task_stats(task, &stats) != 0) return -1;
    if (ztm_task_get_memory_config(task, &config) != 0) return -1;
    
    info->current_usage = stats.memory.memory_used;
    info->current_limit = config.memory_limit;
    info->is_auto = (config.memory_limit == 0);
    
    /* Recommended: 2x current usage, min 128MB, max 1GB */
    info->recommended_limit = stats.memory.memory_used * 2;
    if (info->recommended_limit < 128 * 1024 * 1024) {
        info->recommended_limit = 128 * 1024 * 1024;
    }
    if (info->recommended_limit > 1024ULL * 1024 * 1024) {
        info->recommended_limit = 1024ULL * 1024 * 1024;
    }
    
    info->min_limit = 64 * 1024 * 1024;      /* 64 MB min */
    info->max_limit = 2048ULL * 1024 * 1024; /* 2 GB max */
    
    return 0;
}

int ztm_ui_set_memory_limit(ztm_task_t task, uint64_t limit_bytes) {
    return ztm_task_set_memory_limit(task, limit_bytes);
}

int ztm_ui_set_memory_auto(ztm_task_t task) {
    return ztm_task_set_memory_limit(task, 0);
}

/* ===========================================================================
 * System Summary
 * =========================================================================*/

int ztm_ui_get_system_summary(ztm_system_summary_t *summary) {
    if (!summary) return -1;
    
    memset(summary, 0, sizeof(ztm_system_summary_t));
    
    ztm_system_stats_t sys;
    ztm_system_stats(&sys);
    
    summary->cpu_usage = sys.cpu_usage_total;
    summary->cpu_cores = sys.cpu_cores;
    summary->memory_total = sys.memory_total;
    summary->memory_used = sys.memory_used;
    summary->memory_browser = sys.memory_browser;
    summary->memory_pressure = zam_memory_pressure();
    
    /* Format summaries */
    snprintf(summary->cpu_summary, sizeof(summary->cpu_summary),
             "CPU: %.0f%%", sys.cpu_usage_total);
    
    char used_str[16], total_str[16], browser_str[16];
    format_bytes(sys.memory_used, used_str, sizeof(used_str));
    format_bytes(sys.memory_total, total_str, sizeof(total_str));
    format_bytes(sys.memory_browser, browser_str, sizeof(browser_str));
    
    snprintf(summary->memory_summary, sizeof(summary->memory_summary),
             "Memory: %s / %s (%.0f%%)", 
             used_str, total_str,
             (double)sys.memory_used / sys.memory_total * 100);
    
    snprintf(summary->browser_summary, sizeof(summary->browser_summary),
             "Browser: %s", browser_str);
    
    /* Task counts */
    ztm_task_t tasks[256];
    int count = ztm_task_list(tasks, 256);
    summary->total_tasks = count;
    
    for (int i = 0; i < count; i++) {
        ztm_task_info_t info;
        if (ztm_task_info(tasks[i], &info) == 0) {
            if (info.state == ZTM_STATE_RUNNING) {
                summary->running_tasks++;
            } else if (info.state == ZTM_STATE_SUSPENDED) {
                summary->suspended_tasks++;
            }
        }
    }
    
    /* Status */
    summary->auto_manage_enabled = zam_is_enabled();
    summary->memory_warning = summary->memory_pressure > 0.75;
    summary->memory_critical = summary->memory_pressure > 0.90;
    
    return 0;
}
