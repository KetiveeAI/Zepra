/*
 * ZepraBrowser Task Auto-Manager
 * 
 * Automatic memory and process management with:
 * - Intelligent memory allocation
 * - Auto-suspend inactive tabs
 * - Auto-kill high memory tabs
 * - Background tab optimization
 * 
 * Copyright (c) 2025 KetiveeAI - KETIVEEAI License
 */

#ifndef ZEPRA_AUTO_MANAGER_H
#define ZEPRA_AUTO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "browser_core/task_manager.h"

/* ============ Auto Manager Config ============ */
typedef struct {
    /* Memory management */
    uint64_t    memory_warning_threshold;   /* Warn when system memory low */
    uint64_t    memory_critical_threshold;  /* Start killing tabs */
    uint64_t    per_tab_memory_soft_limit;  /* Soft limit per tab */
    uint64_t    per_tab_memory_hard_limit;  /* Hard limit (kill if exceeded) */
    
    /* Tab management */
    int         auto_suspend_enabled;       /* Auto-suspend background tabs */
    int         auto_suspend_delay_sec;     /* Seconds before suspending */
    int         auto_discard_enabled;       /* Discard old tabs to save memory */
    int         max_active_tabs;            /* Max tabs before auto-suspend */
    
    /* CPU management */
    double      cpu_throttle_threshold;     /* Throttle if CPU > threshold */
    int         cpu_throttle_enabled;
    
    /* Priority */
    int         prioritize_active_tab;      /* Give active tab more resources */
    int         prioritize_audio_tabs;      /* Don't suspend tabs with audio */
    int         prioritize_video_tabs;      /* Don't suspend tabs with video */
    
} zam_config_t;

/* ============ Tab Importance ============ */
typedef enum {
    ZAM_IMPORTANCE_CRITICAL    = 5,  /* Pinned, active, or has unsaved data */
    ZAM_IMPORTANCE_HIGH        = 4,  /* Recently used, playing media */
    ZAM_IMPORTANCE_NORMAL      = 3,  /* Regular tabs */
    ZAM_IMPORTANCE_LOW         = 2,  /* Background tabs */
    ZAM_IMPORTANCE_DISCARDABLE = 1,  /* Old tabs, safe to discard */
} zam_importance_t;

/* ============ Auto Manager API ============ */

/**
 * Initialize auto manager
 */
int zam_init(const zam_config_t *config);

/**
 * Shutdown auto manager
 */
void zam_shutdown(void);

/**
 * Get/set config
 */
void zam_get_config(zam_config_t *config);
void zam_set_config(const zam_config_t *config);

/**
 * Enable/disable auto management
 */
void zam_enable(int enabled);
int zam_is_enabled(void);

/**
 * Get memory pressure level (0.0-1.0)
 */
double zam_memory_pressure(void);

/**
 * Get tab importance
 */
zam_importance_t zam_get_importance(ztm_task_t task);

/**
 * Set tab as pinned (won't be auto-suspended)
 */
void zam_set_pinned(ztm_task_t task, int pinned);

/**
 * Mark tab as having unsaved data
 */
void zam_set_unsaved(ztm_task_t task, int unsaved);

/**
 * Force memory optimization
 */
void zam_optimize_now(void);

/**
 * Get recommended action for task
 */
typedef enum {
    ZAM_ACTION_NONE,
    ZAM_ACTION_THROTTLE,
    ZAM_ACTION_SUSPEND,
    ZAM_ACTION_DISCARD,
    ZAM_ACTION_KILL,
} zam_action_t;

zam_action_t zam_get_recommendation(ztm_task_t task);

#ifdef __cplusplus
}
#endif

#endif /* ZEPRA_AUTO_MANAGER_H */
