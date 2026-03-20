// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "nxbase.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// NxArena — Bump allocator for parser temporaries
// ============================================================================

#define NX_ARENA_BLOCK_SIZE  (64 * 1024)  // 64KB blocks
#define NX_ARENA_ALIGN       8

typedef struct NxArenaBlock {
    struct NxArenaBlock* next;
    size_t               capacity;
    size_t               used;
    // data follows immediately after this struct
} NxArenaBlock;

static uint8_t* arena_block_data(NxArenaBlock* b) {
    return (uint8_t*)(b + 1);
}

struct NxArena {
    NxArenaBlock* head;
    NxArenaBlock* current;
    size_t        total_allocated;
};

static NxArenaBlock* arena_block_create(size_t min_size) {
    size_t cap = min_size > NX_ARENA_BLOCK_SIZE ? min_size : NX_ARENA_BLOCK_SIZE;
    NxArenaBlock* b = (NxArenaBlock*)malloc(sizeof(NxArenaBlock) + cap);
    if (!b) return NULL;
    b->next = NULL;
    b->capacity = cap;
    b->used = 0;
    return b;
}

NxArena* nx_arena_create(void) {
    NxArena* a = (NxArena*)calloc(1, sizeof(NxArena));
    if (!a) return NULL;
    a->head = arena_block_create(NX_ARENA_BLOCK_SIZE);
    if (!a->head) { free(a); return NULL; }
    a->current = a->head;
    return a;
}

void nx_arena_destroy(NxArena* a) {
    if (!a) return;
    NxArenaBlock* b = a->head;
    while (b) {
        NxArenaBlock* next = b->next;
        free(b);
        b = next;
    }
    free(a);
}

void* nx_arena_alloc(NxArena* a, size_t size) {
    if (!a || size == 0) return NULL;

    // align up
    size = (size + NX_ARENA_ALIGN - 1) & ~(NX_ARENA_ALIGN - 1);

    // try current block
    if (a->current->used + size <= a->current->capacity) {
        void* ptr = arena_block_data(a->current) + a->current->used;
        a->current->used += size;
        a->total_allocated += size;
        return ptr;
    }

    // allocate new block
    NxArenaBlock* b = arena_block_create(size);
    if (!b) return NULL;
    a->current->next = b;
    a->current = b;

    void* ptr = arena_block_data(b);
    b->used = size;
    a->total_allocated += size;
    return ptr;
}

char* nx_arena_strdup(NxArena* a, const char* str) {
    if (!a || !str) return NULL;
    size_t len = strlen(str) + 1;
    char* copy = (char*)nx_arena_alloc(a, len);
    if (copy) memcpy(copy, str, len);
    return copy;
}

char* nx_arena_strndup(NxArena* a, const char* str, size_t len) {
    if (!a || !str) return NULL;
    char* copy = (char*)nx_arena_alloc(a, len + 1);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

void nx_arena_reset(NxArena* a) {
    if (!a) return;
    // free all blocks except first
    NxArenaBlock* b = a->head->next;
    while (b) {
        NxArenaBlock* next = b->next;
        free(b);
        b = next;
    }
    a->head->next = NULL;
    a->head->used = 0;
    a->current = a->head;
    a->total_allocated = 0;
}

size_t nx_arena_total(const NxArena* a) {
    return a ? a->total_allocated : 0;
}
