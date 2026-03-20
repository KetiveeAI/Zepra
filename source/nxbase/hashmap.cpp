// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "nxbase.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// NxHashMap — Open-addressing, FNV-1a hashed, string-keyed
// ============================================================================

#define NXHM_INITIAL_CAP  16
#define NXHM_LOAD_FACTOR  0.75
#define NXHM_TOMBSTONE    ((char*)(uintptr_t)1)

typedef struct {
    char*  key;
    void*  value;
} NxHashEntry;

struct NxHashMap {
    NxHashEntry* entries;
    size_t       capacity;
    size_t       count;      // live entries
    size_t       occupied;   // live + tombstones
    NxFreeFunc   value_free;
};

static uint64_t fnv1a(const char* key) {
    uint64_t h = 14695981039346656037ULL;
    for (const uint8_t* p = (const uint8_t*)key; *p; ++p) {
        h ^= *p;
        h *= 1099511628211ULL;
    }
    return h;
}

NxHashMap* nx_hashmap_create(size_t initial_capacity, NxFreeFunc value_free) {
    NxHashMap* m = (NxHashMap*)calloc(1, sizeof(NxHashMap));
    if (!m) return NULL;
    if (initial_capacity < NXHM_INITIAL_CAP) initial_capacity = NXHM_INITIAL_CAP;
    // round up to power of 2
    size_t cap = 1;
    while (cap < initial_capacity) cap <<= 1;
    m->entries = (NxHashEntry*)calloc(cap, sizeof(NxHashEntry));
    if (!m->entries) { free(m); return NULL; }
    m->capacity = cap;
    m->value_free = value_free;
    return m;
}

void nx_hashmap_destroy(NxHashMap* m) {
    if (!m) return;
    for (size_t i = 0; i < m->capacity; ++i) {
        if (m->entries[i].key && m->entries[i].key != NXHM_TOMBSTONE) {
            free(m->entries[i].key);
            if (m->value_free) m->value_free(m->entries[i].value);
        }
    }
    free(m->entries);
    free(m);
}

static NxResult nx_hashmap_resize(NxHashMap* m, size_t new_cap) {
    NxHashEntry* old = m->entries;
    size_t old_cap = m->capacity;
    m->entries = (NxHashEntry*)calloc(new_cap, sizeof(NxHashEntry));
    if (!m->entries) { m->entries = old; return NX_ERROR_NOMEM; }
    m->capacity = new_cap;
    m->count = 0;
    m->occupied = 0;
    for (size_t i = 0; i < old_cap; ++i) {
        if (old[i].key && old[i].key != NXHM_TOMBSTONE) {
            nx_hashmap_set(m, old[i].key, old[i].value);
            free(old[i].key);
        }
    }
    free(old);
    return NX_OK;
}

NxResult nx_hashmap_set(NxHashMap* m, const char* key, void* value) {
    if (!m || !key) return NX_ERROR_INVALID;
    if ((double)m->occupied / m->capacity >= NXHM_LOAD_FACTOR) {
        NxResult r = nx_hashmap_resize(m, m->capacity * 2);
        if (r != NX_OK) return r;
    }
    uint64_t h = fnv1a(key);
    size_t mask = m->capacity - 1;
    size_t idx = h & mask;
    size_t first_tomb = (size_t)-1;
    for (;;) {
        NxHashEntry* e = &m->entries[idx];
        if (!e->key) {
            // empty slot
            size_t target = (first_tomb != (size_t)-1) ? first_tomb : idx;
            NxHashEntry* t = &m->entries[target];
            t->key = strdup(key);
            if (!t->key) return NX_ERROR_NOMEM;
            t->value = value;
            m->count++;
            if (first_tomb == (size_t)-1) m->occupied++;
            return NX_OK;
        }
        if (e->key == NXHM_TOMBSTONE) {
            if (first_tomb == (size_t)-1) first_tomb = idx;
        } else if (strcmp(e->key, key) == 0) {
            // overwrite
            if (m->value_free && e->value != value) m->value_free(e->value);
            e->value = value;
            return NX_OK;
        }
        idx = (idx + 1) & mask;
    }
}

void* nx_hashmap_get(const NxHashMap* m, const char* key) {
    if (!m || !key) return NULL;
    uint64_t h = fnv1a(key);
    size_t mask = m->capacity - 1;
    size_t idx = h & mask;
    for (;;) {
        const NxHashEntry* e = &m->entries[idx];
        if (!e->key) return NULL;
        if (e->key != NXHM_TOMBSTONE && strcmp(e->key, key) == 0) {
            return e->value;
        }
        idx = (idx + 1) & mask;
    }
}

bool nx_hashmap_has(const NxHashMap* m, const char* key) {
    if (!m || !key) return false;
    uint64_t h = fnv1a(key);
    size_t mask = m->capacity - 1;
    size_t idx = h & mask;
    for (;;) {
        const NxHashEntry* e = &m->entries[idx];
        if (!e->key) return false;
        if (e->key != NXHM_TOMBSTONE && strcmp(e->key, key) == 0) return true;
        idx = (idx + 1) & mask;
    }
}

bool nx_hashmap_remove(NxHashMap* m, const char* key) {
    if (!m || !key) return false;
    uint64_t h = fnv1a(key);
    size_t mask = m->capacity - 1;
    size_t idx = h & mask;
    for (;;) {
        NxHashEntry* e = &m->entries[idx];
        if (!e->key) return false;
        if (e->key != NXHM_TOMBSTONE && strcmp(e->key, key) == 0) {
            free(e->key);
            if (m->value_free) m->value_free(e->value);
            e->key = NXHM_TOMBSTONE;
            e->value = NULL;
            m->count--;
            return true;
        }
        idx = (idx + 1) & mask;
    }
}

size_t nx_hashmap_count(const NxHashMap* m) {
    return m ? m->count : 0;
}

void nx_hashmap_clear(NxHashMap* m) {
    if (!m) return;
    for (size_t i = 0; i < m->capacity; ++i) {
        if (m->entries[i].key && m->entries[i].key != NXHM_TOMBSTONE) {
            free(m->entries[i].key);
            if (m->value_free) m->value_free(m->entries[i].value);
        }
        m->entries[i].key = NULL;
        m->entries[i].value = NULL;
    }
    m->count = 0;
    m->occupied = 0;
}

NxResult nx_hashmap_iterate(const NxHashMap* m, NxHashMapIterFunc fn, void* ctx) {
    if (!m || !fn) return NX_ERROR_INVALID;
    for (size_t i = 0; i < m->capacity; ++i) {
        if (m->entries[i].key && m->entries[i].key != NXHM_TOMBSTONE) {
            if (!fn(m->entries[i].key, m->entries[i].value, ctx)) break;
        }
    }
    return NX_OK;
}
