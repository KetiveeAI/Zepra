/*
 * ZepraBrowser Task Manager UI
 * 
 * User interface for task manager with:
 * - Normal mode: Kill, Isolate
 * - Advanced mode: RAM allocation, detailed stats
 * 
 * Copyright (c) 2025 KetiveeAI - KETIVEEAI License
 */

#ifndef ZEPRA_TASK_MANAGER_UI_H
#define ZEPRA_TASK_MANAGER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "browser_core/task_manager.h"
#include "browser_core/auto_manager.h"

/* ============ UI Mode ============ */
typedef enum {
    ZTM_UI_MODE_NORMAL   = 0,  /* Kill, Isolate only */
    ZTM_UI_MODE_ADVANCED = 1,  /* Full controls (RAM, priority, etc) */
} ztm_ui_mode_t;

/* ============ Column Visibility ============ */
typedef struct {
    uint8_t show_title;
    uint8_t show_type;
    uint8_t show_cpu;
    uint8_t show_memory;
    uint8_t show_network;
    uint8_t show_process_id;
    uint8_t show_state;
    uint8_t show_priority;      /* Advanced only */
    uint8_t show_memory_limit;  /* Advanced only */
    uint8_t show_isolation;     /* Advanced only */
} ztm_ui_columns_t;

/* ============ Sort Options ============ */
typedef enum {
    ZTM_SORT_TITLE,
    ZTM_SORT_CPU,
    ZTM_SORT_MEMORY,
    ZTM_SORT_NETWORK,
    ZTM_SORT_TYPE,
} ztm_sort_t;

/* ============ UI State ============ */
typedef struct {
    ztm_ui_mode_t   mode;
    ztm_ui_columns_t columns;
    ztm_sort_t      sort_by;
    int             sort_descending;
    ztm_task_t      selected_task;
    int             refresh_interval_ms;
    int             auto_refresh;
} ztm_ui_state_t;

/* ============ UI Row Data ============ */
typedef struct {
    ztm_task_t      id;
    char            title[256];
    char            type_str[32];
    char            state_str[32];
    
    /* Formatted stats */
    char            cpu_str[16];        /* "12.5%" */
    char            memory_str[32];     /* "125 MB / 256 MB" */
    char            memory_limit_str[16]; /* "256 MB" or "Auto" */
    char            network_str[32];    /* "1.2 MB ↓ 0.5 MB ↑" */
    char            priority_str[16];   /* "Normal" */
    
    /* Raw values for sorting */
    double          cpu_usage;
    uint64_t        memory_used;
    uint64_t        memory_limit;
    uint64_t        network_total;
    
    /* Flags */
    uint8_t         is_playing_audio;
    uint8_t         is_playing_video;
    uint8_t         is_isolated;
    uint8_t         is_sandboxed;
    uint8_t         is_frozen;
    uint8_t         is_pinned;
    
    /* Colors */
    uint8_t         memory_warning;     /* Memory usage > 75% of limit */
    uint8_t         memory_critical;    /* Memory usage > 90% of limit */
    uint8_t         cpu_warning;        /* CPU > 50% */
    uint8_t         cpu_critical;       /* CPU > 90% */
} ztm_ui_row_t;

/* ============ Action Menu ============ */
typedef enum {
    /* Normal mode actions */
    ZTM_ACTION_END_TASK,
    ZTM_ACTION_ISOLATE,
    ZTM_ACTION_PIN,
    
    /* Advanced mode actions */
    ZTM_ACTION_KILL_FORCE,
    ZTM_ACTION_PAUSE,
    ZTM_ACTION_RESUME,
    ZTM_ACTION_SUSPEND,
    ZTM_ACTION_SET_PRIORITY,
    ZTM_ACTION_SET_MEMORY_LIMIT,
    ZTM_ACTION_TRIM_MEMORY,
    ZTM_ACTION_SANDBOX,
} ztm_ui_action_t;

/* ============ UI Callbacks ============ */
typedef void (*ztm_ui_refresh_cb)(void *userdata, const ztm_ui_row_t *rows, int count);
typedef void (*ztm_ui_selected_cb)(void *userdata, ztm_task_t task);
typedef void (*ztm_ui_action_cb)(void *userdata, ztm_task_t task, ztm_ui_action_t action);

/* ===========================================================================
 * Task Manager UI API
 * =========================================================================*/

/**
 * Initialize UI state
 */
void ztm_ui_init(ztm_ui_state_t *state);

/**
 * Set UI mode (normal/advanced)
 */
void ztm_ui_set_mode(ztm_ui_state_t *state, ztm_ui_mode_t mode);

/**
 * Check if advanced mode requires unlock
 */
int ztm_ui_needs_advanced_unlock(void);

/**
 * Unlock advanced mode (returns 1 if successful)
 */
int ztm_ui_unlock_advanced(const char *password);

/**
 * Get row data for all tasks
 */
int ztm_ui_get_rows(const ztm_ui_state_t *state, ztm_ui_row_t *rows, int max_rows);

/**
 * Perform action on task
 */
int ztm_ui_perform_action(ztm_ui_action_t action, ztm_task_t task, void *param);

/**
 * Get available actions for current mode
 */
int ztm_ui_get_available_actions(ztm_ui_mode_t mode, ztm_ui_action_t *actions, int max);

/**
 * Get action name for display
 */
const char* ztm_ui_action_name(ztm_ui_action_t action);

/**
 * Check if action is available in current mode
 */
int ztm_ui_action_available(ztm_ui_mode_t mode, ztm_ui_action_t action);

/* ===========================================================================
 * Memory Allocation UI (Advanced Mode)
 * =========================================================================*/

typedef struct {
    uint64_t    current_usage;
    uint64_t    current_limit;      /* 0 = Auto */
    uint64_t    recommended_limit;
    uint64_t    min_limit;
    uint64_t    max_limit;
    int         is_auto;
} ztm_memory_ui_t;

/**
 * Get memory allocation UI data for task
 */
int ztm_ui_get_memory_info(ztm_task_t task, ztm_memory_ui_t *info);

/**
 * Set memory limit for task (0 = Auto)
 */
int ztm_ui_set_memory_limit(ztm_task_t task, uint64_t limit_bytes);

/**
 * Set memory limit to auto
 */
int ztm_ui_set_memory_auto(ztm_task_t task);

/* ===========================================================================
 * System Overview (Header bar)
 * =========================================================================*/

typedef struct {
    /* CPU */
    double      cpu_usage;
    int         cpu_cores;
    
    /* Memory */
    uint64_t    memory_total;
    uint64_t    memory_used;
    uint64_t    memory_browser;
    double      memory_pressure;    /* 0.0 - 1.0 */
    
    /* Summary */
    char        cpu_summary[32];    /* "CPU: 45%" */
    char        memory_summary[64]; /* "Memory: 4.2 GB / 16 GB (26%)" */
    char        browser_summary[32];/* "Browser: 1.2 GB" */
    
    /* Tasks */
    int         total_tasks;
    int         running_tasks;
    int         suspended_tasks;
    
    /* Status */
    uint8_t     auto_manage_enabled;
    uint8_t     memory_warning;
    uint8_t     memory_critical;
} ztm_system_summary_t;

/**
 * Get system summary for header bar
 */
int ztm_ui_get_system_summary(ztm_system_summary_t *summary);

#ifdef __cplusplus
}
#endif

#endif /* ZEPRA_TASK_MANAGER_UI_H */
