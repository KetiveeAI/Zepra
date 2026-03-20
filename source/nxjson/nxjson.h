// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file nxjson.h
 * @brief NxJSON - Fast, lightweight JSON parser for Zepra
 * 
 * Replaces nlohmann/json with a minimal, high-performance implementation.
 * 
 * Features:
 * - Zero-copy parsing where possible
 * - SAX and DOM parsing modes
 * - JSON5 support (optional)
 * - Pretty printing
 */

#ifndef NXJSON_H
#define NXJSON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef enum {
    NX_JSON_NULL = 0,
    NX_JSON_BOOL,
    NX_JSON_NUMBER,
    NX_JSON_STRING,
    NX_JSON_ARRAY,
    NX_JSON_OBJECT
} NxJsonType;

typedef struct NxJsonValue NxJsonValue;

// ============================================================================
// Parsing
// ============================================================================

typedef enum {
    NX_JSON_OK = 0,
    NX_JSON_ERROR_NOMEM = -1,
    NX_JSON_ERROR_SYNTAX = -2,
    NX_JSON_ERROR_UNEXPECTED_TOKEN = -3,
    NX_JSON_ERROR_UNEXPECTED_EOF = -4,
    NX_JSON_ERROR_INVALID_ESCAPE = -5,
    NX_JSON_ERROR_INVALID_NUMBER = -6,
    NX_JSON_ERROR_DEPTH_LIMIT = -7
} NxJsonError;

typedef struct {
    int line;
    int column;
    const char* message;
    NxJsonError code;
} NxJsonParseError;

/**
 * Parse JSON string into a value tree.
 * @param json  JSON string (null-terminated)
 * @param error Optional error output
 * @return Parsed value or NULL on error. Caller must call nx_json_free().
 */
NxJsonValue* nx_json_parse(const char* json, NxJsonParseError* error);

/**
 * Parse JSON string with length (not null-terminated).
 */
NxJsonValue* nx_json_parse_len(const char* json, size_t len, NxJsonParseError* error);

/**
 * Free a parsed JSON value tree.
 */
void nx_json_free(NxJsonValue* value);

// ============================================================================
// Value Access
// ============================================================================

NxJsonType nx_json_type(const NxJsonValue* value);
bool nx_json_is_null(const NxJsonValue* value);
bool nx_json_is_bool(const NxJsonValue* value);
bool nx_json_is_number(const NxJsonValue* value);
bool nx_json_is_string(const NxJsonValue* value);
bool nx_json_is_array(const NxJsonValue* value);
bool nx_json_is_object(const NxJsonValue* value);

bool nx_json_get_bool(const NxJsonValue* value);
double nx_json_get_number(const NxJsonValue* value);
int64_t nx_json_get_int(const NxJsonValue* value);
const char* nx_json_get_string(const NxJsonValue* value);
size_t nx_json_get_string_len(const NxJsonValue* value);

// Array access
size_t nx_json_array_size(const NxJsonValue* value);
NxJsonValue* nx_json_array_get(const NxJsonValue* value, size_t index);

// Object access
size_t nx_json_object_size(const NxJsonValue* value);
NxJsonValue* nx_json_object_get(const NxJsonValue* value, const char* key);
bool nx_json_object_has(const NxJsonValue* value, const char* key);

// Object iteration
typedef struct {
    const char* key;
    NxJsonValue* value;
} NxJsonMember;

size_t nx_json_object_count(const NxJsonValue* value);
NxJsonMember nx_json_object_at(const NxJsonValue* value, size_t index);

// ============================================================================
// Value Creation
// ============================================================================

NxJsonValue* nx_json_null(void);
NxJsonValue* nx_json_bool(bool value);
NxJsonValue* nx_json_number(double value);
NxJsonValue* nx_json_int(int64_t value);
NxJsonValue* nx_json_string(const char* value);
NxJsonValue* nx_json_string_len(const char* value, size_t len);
NxJsonValue* nx_json_array(void);
NxJsonValue* nx_json_object(void);

// Mutation
void nx_json_array_push(NxJsonValue* array, NxJsonValue* value);
void nx_json_object_set(NxJsonValue* object, const char* key, NxJsonValue* value);

// ============================================================================
// Serialization
// ============================================================================

typedef struct {
    bool pretty;           // Enable pretty printing
    int indent_size;       // Spaces per indent level (default: 2)
    bool sort_keys;        // Sort object keys
    bool escape_unicode;   // Escape non-ASCII as \uXXXX
} NxJsonWriteOptions;

/**
 * Serialize JSON value to string.
 * @param value   Value to serialize
 * @param options Optional formatting options (NULL for compact)
 * @return Allocated string. Caller must free().
 */
char* nx_json_stringify(const NxJsonValue* value, const NxJsonWriteOptions* options);

/**
 * Serialize JSON value with default options (compact).
 */
char* nx_json_to_string(const NxJsonValue* value);

// ============================================================================
// JSON Pointer (RFC 6901)
// ============================================================================

/**
 * Access nested value by JSON Pointer path.
 * @param root    Root value to query
 * @param pointer Path string, e.g. "/user/settings/theme"
 * @return Value at path, or NULL if not found. Not owned by caller.
 */
NxJsonValue* nx_json_pointer_get(const NxJsonValue* root, const char* pointer);

// ============================================================================
// Deep Copy & Comparison
// ============================================================================

NxJsonValue* nx_json_clone(const NxJsonValue* value);
bool nx_json_equal(const NxJsonValue* a, const NxJsonValue* b);

// ============================================================================
// Extended Mutation
// ============================================================================

NxJsonValue* nx_json_array_pop(NxJsonValue* array);
size_t nx_json_object_keys(const NxJsonValue* obj, const char** keys, size_t max_keys);

#ifdef __cplusplus
}
#endif

// ============================================================================
// C++ Wrapper
// ============================================================================

#ifdef __cplusplus
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

namespace nx {

class JsonException : public std::runtime_error {
public:
    explicit JsonException(const std::string& msg) : std::runtime_error(msg) {}
};

class Json {
public:
    Json() : value_(nx_json_null()), owned_(true) {}
    ~Json() { if (owned_ && value_) nx_json_free(value_); }
    
    // Move
    Json(Json&& other) noexcept : value_(other.value_), owned_(other.owned_) {
        other.value_ = nullptr;
        other.owned_ = false;
    }
    Json& operator=(Json&& other) noexcept {
        if (this != &other) {
            if (owned_ && value_) nx_json_free(value_);
            value_ = other.value_;
            owned_ = other.owned_;
            other.value_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    // No copy
    Json(const Json&) = delete;
    Json& operator=(const Json&) = delete;
    
    // Parse
    static Json parse(const std::string& json) {
        NxJsonParseError error;
        NxJsonValue* val = nx_json_parse(json.c_str(), &error);
        if (!val) {
            throw JsonException(std::string("JSON parse error: ") + error.message);
        }
        return Json(val, true);
    }
    
    static Json parse(std::string_view json) {
        NxJsonParseError error;
        NxJsonValue* val = nx_json_parse_len(json.data(), json.size(), &error);
        if (!val) {
            throw JsonException(std::string("JSON parse error: ") + error.message);
        }
        return Json(val, true);
    }
    
    // Factory methods
    static Json null() { return Json(nx_json_null(), true); }
    static Json boolean(bool v) { return Json(nx_json_bool(v), true); }
    static Json number(double v) { return Json(nx_json_number(v), true); }
    static Json string(const std::string& v) { return Json(nx_json_string(v.c_str()), true); }
    static Json array() { return Json(nx_json_array(), true); }
    static Json object() { return Json(nx_json_object(), true); }
    
    // Type checking
    NxJsonType type() const { return nx_json_type(value_); }
    bool isNull() const { return nx_json_is_null(value_); }
    bool isBool() const { return nx_json_is_bool(value_); }
    bool isNumber() const { return nx_json_is_number(value_); }
    bool isString() const { return nx_json_is_string(value_); }
    bool isArray() const { return nx_json_is_array(value_); }
    bool isObject() const { return nx_json_is_object(value_); }
    
    // Value access
    bool asBool() const { return nx_json_get_bool(value_); }
    double asNumber() const { return nx_json_get_number(value_); }
    int64_t asInt() const { return nx_json_get_int(value_); }
    std::string asString() const { 
        return std::string(nx_json_get_string(value_), nx_json_get_string_len(value_)); 
    }
    
    // Array access
    size_t size() const { 
        return isArray() ? nx_json_array_size(value_) : nx_json_object_size(value_); 
    }
    Json operator[](size_t index) const {
        return Json(nx_json_array_get(value_, index), false);
    }
    
    // Object access
    Json operator[](const std::string& key) const {
        return Json(nx_json_object_get(value_, key.c_str()), false);
    }
    bool has(const std::string& key) const {
        return nx_json_object_has(value_, key.c_str());
    }
    
    // Serialization
    std::string dump(int indent = -1) const {
        if (indent < 0) {
            char* s = nx_json_to_string(value_);
            std::string result(s);
            free(s);
            return result;
        }
        NxJsonWriteOptions opts = {};
        opts.pretty = true;
        opts.indent_size = indent;
        char* s = nx_json_stringify(value_, &opts);
        std::string result(s);
        free(s);
        return result;
    }
    
private:
    Json(NxJsonValue* value, bool owned) : value_(value), owned_(owned) {}
    
    NxJsonValue* value_;
    bool owned_;
};

} // namespace nx

#endif // __cplusplus

#endif // NXJSON_H
