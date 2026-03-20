// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

// pointer.cpp — JSON Pointer (RFC 6901) path-based access
// Supports: /foo/bar/0, /a~0b (escaped ~), /a~1b (escaped /)

#include "nxjson.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Unescape JSON Pointer token: ~0 -> ~, ~1 -> /
static char* unescape_token(const char* token, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; ++i) {
        if (token[i] == '~' && i + 1 < len) {
            if (token[i + 1] == '0') { out[j++] = '~'; i++; continue; }
            if (token[i + 1] == '1') { out[j++] = '/'; i++; continue; }
        }
        out[j++] = token[i];
    }
    out[j] = '\0';
    return out;
}

static bool is_array_index(const char* token, size_t* out_index) {
    if (!token || !*token) return false;
    if (token[0] == '0' && token[1] != '\0') return false;  // no leading zeros
    char* end = NULL;
    unsigned long val = strtoul(token, &end, 10);
    if (*end != '\0') return false;
    *out_index = (size_t)val;
    return true;
}

NxJsonValue* nx_json_pointer_get(const NxJsonValue* root, const char* pointer) {
    if (!root || !pointer) return NULL;
    if (*pointer == '\0') return (NxJsonValue*)root;
    if (*pointer != '/') return NULL;  // must start with /

    const NxJsonValue* current = root;
    const char* p = pointer + 1;

    while (*p && current) {
        const char* slash = strchr(p, '/');
        size_t tok_len = slash ? (size_t)(slash - p) : strlen(p);

        char* token = unescape_token(p, tok_len);
        if (!token) return NULL;

        if (nx_json_is_object(current)) {
            current = nx_json_object_get(current, token);
        } else if (nx_json_is_array(current)) {
            size_t idx;
            if (is_array_index(token, &idx)) {
                current = nx_json_array_get(current, idx);
            } else {
                free(token);
                return NULL;
            }
        } else {
            free(token);
            return NULL;
        }

        free(token);
        p += tok_len;
        if (*p == '/') p++;
    }

    return (NxJsonValue*)current;
}

// Deep copy
NxJsonValue* nx_json_clone(const NxJsonValue* value) {
    if (!value) return nx_json_null();

    switch (nx_json_type(value)) {
        case NX_JSON_NULL:   return nx_json_null();
        case NX_JSON_BOOL:   return nx_json_bool(nx_json_get_bool(value));
        case NX_JSON_NUMBER: return nx_json_number(nx_json_get_number(value));
        case NX_JSON_STRING: return nx_json_string_len(
            nx_json_get_string(value), nx_json_get_string_len(value));
        case NX_JSON_ARRAY: {
            NxJsonValue* arr = nx_json_array();
            for (size_t i = 0; i < nx_json_array_size(value); ++i) {
                nx_json_array_push(arr, nx_json_clone(nx_json_array_get(value, i)));
            }
            return arr;
        }
        case NX_JSON_OBJECT: {
            NxJsonValue* obj = nx_json_object();
            for (size_t i = 0; i < nx_json_object_count(value); ++i) {
                NxJsonMember m = nx_json_object_at(value, i);
                nx_json_object_set(obj, m.key, nx_json_clone(m.value));
            }
            return obj;
        }
    }
    return nx_json_null();
}

// Equality
bool nx_json_equal(const NxJsonValue* a, const NxJsonValue* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (nx_json_type(a) != nx_json_type(b)) return false;

    switch (nx_json_type(a)) {
        case NX_JSON_NULL: return true;
        case NX_JSON_BOOL: return nx_json_get_bool(a) == nx_json_get_bool(b);
        case NX_JSON_NUMBER: return nx_json_get_number(a) == nx_json_get_number(b);
        case NX_JSON_STRING: {
            size_t la = nx_json_get_string_len(a);
            size_t lb = nx_json_get_string_len(b);
            if (la != lb) return false;
            return memcmp(nx_json_get_string(a), nx_json_get_string(b), la) == 0;
        }
        case NX_JSON_ARRAY: {
            size_t sa = nx_json_array_size(a);
            if (sa != nx_json_array_size(b)) return false;
            for (size_t i = 0; i < sa; ++i) {
                if (!nx_json_equal(nx_json_array_get(a, i), nx_json_array_get(b, i)))
                    return false;
            }
            return true;
        }
        case NX_JSON_OBJECT: {
            size_t ca = nx_json_object_count(a);
            if (ca != nx_json_object_count(b)) return false;
            for (size_t i = 0; i < ca; ++i) {
                NxJsonMember ma = nx_json_object_at(a, i);
                NxJsonValue* vb = nx_json_object_get(b, ma.key);
                if (!vb || !nx_json_equal(ma.value, vb)) return false;
            }
            return true;
        }
    }
    return false;
}

// Array mutation
NxJsonValue* nx_json_array_pop(NxJsonValue* array) {
    if (!array || !nx_json_is_array(array)) return NULL;
    size_t sz = nx_json_array_size(array);
    if (sz == 0) return NULL;
    return nx_json_array_get(array, sz - 1);
    // NOTE: actual removal from backing store requires internal access
    // For now, returns last element. Full removal needs parser.cpp internal API.
}

// Object key listing
size_t nx_json_object_keys(const NxJsonValue* obj, const char** keys, size_t max_keys) {
    if (!obj || !nx_json_is_object(obj) || !keys) return 0;
    size_t count = nx_json_object_count(obj);
    size_t n = count < max_keys ? count : max_keys;
    for (size_t i = 0; i < n; ++i) {
        NxJsonMember m = nx_json_object_at(obj, i);
        keys[i] = m.key;
    }
    return n;
}
