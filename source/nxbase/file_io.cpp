// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "nxbase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

// ============================================================================
// File I/O — Buffer-based synchronous operations
// ============================================================================

NxResult nx_file_read_all(const char* path, NxBuffer* out) {
    if (!path || !out) return NX_ERROR_INVALID;

    FILE* f = fopen(path, "rb");
    if (!f) return NX_ERROR_IO;

    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NX_ERROR_IO;
    }

    size_t file_size = (size_t)st.st_size;
    NxResult r = nx_buffer_reserve(out, out->size + file_size);
    if (r != NX_OK) { fclose(f); return r; }

    size_t read = fread(out->data + out->size, 1, file_size, f);
    fclose(f);

    if (read != file_size) return NX_ERROR_IO;
    out->size += read;
    return NX_OK;
}

NxResult nx_file_write(const char* path, const void* data, size_t len) {
    if (!path || (!data && len > 0)) return NX_ERROR_INVALID;

    FILE* f = fopen(path, "wb");
    if (!f) return NX_ERROR_IO;

    if (len > 0) {
        size_t written = fwrite(data, 1, len, f);
        if (written != len) {
            fclose(f);
            return NX_ERROR_IO;
        }
    }
    fclose(f);
    return NX_OK;
}

NxResult nx_file_write_string(const char* path, const char* str) {
    return nx_file_write(path, str, str ? strlen(str) : 0);
}

NxResult nx_file_append(const char* path, const void* data, size_t len) {
    if (!path || (!data && len > 0)) return NX_ERROR_INVALID;

    FILE* f = fopen(path, "ab");
    if (!f) return NX_ERROR_IO;

    if (len > 0) {
        size_t written = fwrite(data, 1, len, f);
        if (written != len) {
            fclose(f);
            return NX_ERROR_IO;
        }
    }
    fclose(f);
    return NX_OK;
}

bool nx_file_exists(const char* path) {
    if (!path) return false;
    struct stat st;
    return stat(path, &st) == 0;
}

int64_t nx_file_size(const char* path) {
    if (!path) return -1;
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
}

NxResult nx_file_remove(const char* path) {
    if (!path) return NX_ERROR_INVALID;
    return remove(path) == 0 ? NX_OK : NX_ERROR_IO;
}
