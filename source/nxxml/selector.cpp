// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

// selector.cpp — Basic CSS selector query engine for nxxml
// Supports: tag, .class, #id, [attr], [attr=value], combinators (space, >)

#include "nxxml.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// Selector parsing
// ============================================================================

typedef enum {
    SEL_TAG,        // div
    SEL_CLASS,      // .class
    SEL_ID,         // #id
    SEL_ATTR,       // [attr]
    SEL_ATTR_EQ,    // [attr=value]
} SelectorPartType;

typedef struct {
    SelectorPartType type;
    char value[128];
    char attr_value[128];  // for SEL_ATTR_EQ
} SelectorPart;

typedef struct {
    SelectorPart parts[8];
    int part_count;
    bool direct_child;  // > combinator before this segment
} SelectorSegment;

typedef struct {
    SelectorSegment segments[8];
    int segment_count;
} ParsedSelector;

static void str_lower(char* s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

static bool parse_selector(const char* sel, ParsedSelector* out) {
    memset(out, 0, sizeof(*out));
    if (!sel || !*sel) return false;

    const char* p = sel;
    int seg_idx = 0;

    while (*p && seg_idx < 8) {
        // skip whitespace
        while (*p == ' ') p++;
        if (!*p) break;

        SelectorSegment* seg = &out->segments[seg_idx];

        // check for > combinator
        if (*p == '>') {
            seg->direct_child = true;
            p++;
            while (*p == ' ') p++;
        }

        // parse parts of this compound selector
        while (*p && *p != ' ' && *p != '>' && seg->part_count < 8) {
            SelectorPart* part = &seg->parts[seg->part_count];

            if (*p == '.') {
                // class selector
                p++;
                part->type = SEL_CLASS;
                int i = 0;
                while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ' ' && *p != '>' && i < 127) {
                    part->value[i++] = *p++;
                }
                part->value[i] = '\0';
                seg->part_count++;
            } else if (*p == '#') {
                // id selector
                p++;
                part->type = SEL_ID;
                int i = 0;
                while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ' ' && *p != '>' && i < 127) {
                    part->value[i++] = *p++;
                }
                part->value[i] = '\0';
                seg->part_count++;
            } else if (*p == '[') {
                // attribute selector
                p++;
                int i = 0;
                while (*p && *p != '=' && *p != ']' && i < 127) {
                    part->value[i++] = *p++;
                }
                part->value[i] = '\0';
                if (*p == '=') {
                    part->type = SEL_ATTR_EQ;
                    p++;
                    if (*p == '"' || *p == '\'') p++;
                    i = 0;
                    while (*p && *p != '"' && *p != '\'' && *p != ']' && i < 127) {
                        part->attr_value[i++] = *p++;
                    }
                    part->attr_value[i] = '\0';
                    if (*p == '"' || *p == '\'') p++;
                } else {
                    part->type = SEL_ATTR;
                }
                if (*p == ']') p++;
                seg->part_count++;
            } else {
                // tag selector
                part->type = SEL_TAG;
                int i = 0;
                while (*p && *p != '.' && *p != '#' && *p != '[' && *p != ' ' && *p != '>' && i < 127) {
                    part->value[i++] = *p++;
                }
                part->value[i] = '\0';
                str_lower(part->value);
                seg->part_count++;
            }
        }

        if (seg->part_count > 0) seg_idx++;
    }

    out->segment_count = seg_idx;
    return seg_idx > 0;
}

// ============================================================================
// Matching
// ============================================================================

static bool has_class(NxXmlNode* node, const char* cls) {
    const char* class_attr = nx_xml_node_attr(node, "class");
    if (!class_attr) return false;
    size_t cls_len = strlen(cls);
    const char* p = class_attr;
    while (*p) {
        while (*p == ' ') p++;
        const char* start = p;
        while (*p && *p != ' ') p++;
        size_t len = (size_t)(p - start);
        if (len == cls_len && strncmp(start, cls, cls_len) == 0) return true;
    }
    return false;
}

static bool node_matches_segment(NxXmlNode* node, const SelectorSegment* seg) {
    if (!node || nx_xml_node_type(node) != NX_XML_ELEMENT) return false;

    for (int i = 0; i < seg->part_count; ++i) {
        const SelectorPart* part = &seg->parts[i];
        switch (part->type) {
            case SEL_TAG: {
                const char* name = nx_xml_node_name(node);
                if (!name) return false;
                // case-insensitive compare
                char lower[128];
                strncpy(lower, name, 127);
                lower[127] = '\0';
                str_lower(lower);
                if (strcmp(lower, part->value) != 0) return false;
                break;
            }
            case SEL_CLASS:
                if (!has_class(node, part->value)) return false;
                break;
            case SEL_ID: {
                const char* id = nx_xml_node_attr(node, "id");
                if (!id || strcmp(id, part->value) != 0) return false;
                break;
            }
            case SEL_ATTR:
                if (!nx_xml_node_has_attr(node, part->value)) return false;
                break;
            case SEL_ATTR_EQ: {
                const char* val = nx_xml_node_attr(node, part->value);
                if (!val || strcmp(val, part->attr_value) != 0) return false;
                break;
            }
        }
    }
    return true;
}

static bool node_matches_full(NxXmlNode* node, const ParsedSelector* sel) {
    // Match segments from right to left (last segment must match the node)
    int si = sel->segment_count - 1;
    NxXmlNode* current = node;

    while (si >= 0 && current) {
        if (node_matches_segment(current, &sel->segments[si])) {
            si--;
            if (si >= 0 && sel->segments[si + 1].direct_child) {
                current = nx_xml_node_parent(current);
            } else {
                current = nx_xml_node_parent(current);
            }
        } else {
            if (si < sel->segment_count - 1 && !sel->segments[si + 1].direct_child) {
                // descendant combinator — keep going up
                current = nx_xml_node_parent(current);
            } else {
                return false;
            }
        }
    }
    return si < 0;
}

static void collect_matches(NxXmlNode* node, const ParsedSelector* sel,
                           NxXmlNode*** results, size_t* count, size_t* capacity, bool first_only) {
    if (!node) return;
    if (first_only && *count > 0) return;

    if (nx_xml_node_type(node) == NX_XML_ELEMENT && node_matches_full(node, sel)) {
        if (*count >= *capacity) {
            size_t new_cap = *capacity ? *capacity * 2 : 16;
            NxXmlNode** new_results = (NxXmlNode**)realloc(*results, new_cap * sizeof(NxXmlNode*));
            if (!new_results) return;
            *results = new_results;
            *capacity = new_cap;
        }
        (*results)[(*count)++] = node;
        if (first_only) return;
    }

    for (NxXmlNode* child = nx_xml_node_first_child(node); child; child = nx_xml_node_next_sibling(child)) {
        collect_matches(child, sel, results, count, capacity, first_only);
        if (first_only && *count > 0) return;
    }
}

// ============================================================================
// Public API
// ============================================================================

NxXmlNode* nx_xml_query_selector(NxXmlNode* root, const char* selector) {
    if (!root || !selector) return NULL;

    ParsedSelector parsed;
    if (!parse_selector(selector, &parsed)) return NULL;

    NxXmlNode** results = NULL;
    size_t count = 0, capacity = 0;
    collect_matches(root, &parsed, &results, &count, &capacity, true);

    NxXmlNode* result = (count > 0) ? results[0] : NULL;
    free(results);
    return result;
}

NxXmlNode** nx_xml_query_selector_all(NxXmlNode* root, const char* selector, size_t* out_count) {
    if (out_count) *out_count = 0;
    if (!root || !selector) return NULL;

    ParsedSelector parsed;
    if (!parse_selector(selector, &parsed)) return NULL;

    NxXmlNode** results = NULL;
    size_t count = 0, capacity = 0;
    collect_matches(root, &parsed, &results, &count, &capacity, false);

    if (out_count) *out_count = count;
    return results;  // caller must free()
}
