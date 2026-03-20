// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "nxbase.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// ============================================================================
// NxTimer — Monotonic clock wrapper
// ============================================================================

struct NxTimer {
    struct timespec start;
    struct timespec lap;
    bool running;
};

static void ts_now(struct timespec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

static uint64_t ts_diff_us(const struct timespec* start, const struct timespec* end) {
    int64_t sec = end->tv_sec - start->tv_sec;
    int64_t nsec = end->tv_nsec - start->tv_nsec;
    if (nsec < 0) { sec--; nsec += 1000000000L; }
    return (uint64_t)(sec * 1000000 + nsec / 1000);
}

NxTimer* nx_timer_create(void) {
    NxTimer* t = (NxTimer*)calloc(1, sizeof(NxTimer));
    return t;
}

void nx_timer_destroy(NxTimer* t) {
    free(t);
}

void nx_timer_start(NxTimer* t) {
    if (!t) return;
    ts_now(&t->start);
    t->lap = t->start;
    t->running = true;
}

void nx_timer_stop(NxTimer* t) {
    if (t) t->running = false;
}

void nx_timer_reset(NxTimer* t) {
    if (!t) return;
    memset(&t->start, 0, sizeof(struct timespec));
    memset(&t->lap, 0, sizeof(struct timespec));
    t->running = false;
}

uint64_t nx_timer_elapsed_us(const NxTimer* t) {
    if (!t) return 0;
    struct timespec now;
    ts_now(&now);
    return ts_diff_us(&t->start, &now);
}

uint64_t nx_timer_elapsed_ms(const NxTimer* t) {
    return nx_timer_elapsed_us(t) / 1000;
}

uint64_t nx_timer_lap_us(NxTimer* t) {
    if (!t) return 0;
    struct timespec now;
    ts_now(&now);
    uint64_t elapsed = ts_diff_us(&t->lap, &now);
    t->lap = now;
    return elapsed;
}

uint64_t nx_monotonic_us(void) {
    struct timespec ts;
    ts_now(&ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

uint64_t nx_monotonic_ms(void) {
    return nx_monotonic_us() / 1000;
}
