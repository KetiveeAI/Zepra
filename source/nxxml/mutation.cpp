// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

// mutation.cpp — DOM mutation operations for nxxml
// add/remove children, set/remove/modify attributes

#include "nxxml.h"
#include <stdlib.h>
#include <string.h>

// Internal node structure — must match parser.cpp's internal layout
// Since we don't have access to the struct definition, we declare a
// compatible version. This depends on parser.cpp using the same layout.

typedef struct NxXmlAttr {
    char* name;
    char* value;
} NxXmlAttr;

struct NxXmlNode {
    NxXmlNodeType type;
    char*         name;       // tag name or NULL
    char*         value;      // text content or NULL

    NxXmlAttr*    attrs;
    size_t        attr_count;
    size_t        attr_capacity;

    NxXmlNode*    parent;
    NxXmlNode*    first_child;
    NxXmlNode*    last_child;
    NxXmlNode*    next_sibling;
    NxXmlNode*    prev_sibling;
    size_t        child_count;
};

// ============================================================================
// Attribute Mutation
// ============================================================================

NxXmlError nx_xml_node_set_attr(NxXmlNode* node, const char* name, const char* value) {
    if (!node || !name || node->type != NX_XML_ELEMENT) return NX_XML_ERROR;

    // Check for existing attribute
    for (size_t i = 0; i < node->attr_count; ++i) {
        if (strcmp(node->attrs[i].name, name) == 0) {
            free(node->attrs[i].value);
            node->attrs[i].value = value ? strdup(value) : strdup("");
            return NX_XML_OK;
        }
    }

    // Add new attribute
    if (node->attr_count >= node->attr_capacity) {
        size_t new_cap = node->attr_capacity ? node->attr_capacity * 2 : 4;
        NxXmlAttr* new_attrs = (NxXmlAttr*)realloc(node->attrs, new_cap * sizeof(NxXmlAttr));
        if (!new_attrs) return NX_XML_ERROR_NOMEM;
        node->attrs = new_attrs;
        node->attr_capacity = new_cap;
    }

    node->attrs[node->attr_count].name = strdup(name);
    node->attrs[node->attr_count].value = value ? strdup(value) : strdup("");
    if (!node->attrs[node->attr_count].name) return NX_XML_ERROR_NOMEM;
    node->attr_count++;
    return NX_XML_OK;
}

NxXmlError nx_xml_node_remove_attr(NxXmlNode* node, const char* name) {
    if (!node || !name) return NX_XML_ERROR;

    for (size_t i = 0; i < node->attr_count; ++i) {
        if (strcmp(node->attrs[i].name, name) == 0) {
            free(node->attrs[i].name);
            free(node->attrs[i].value);
            // shift remaining
            for (size_t j = i; j + 1 < node->attr_count; ++j) {
                node->attrs[j] = node->attrs[j + 1];
            }
            node->attr_count--;
            return NX_XML_OK;
        }
    }
    return NX_XML_ERROR_NOT_FOUND;
}

// ============================================================================
// Child Mutation
// ============================================================================

NxXmlError nx_xml_node_append_child(NxXmlNode* parent, NxXmlNode* child) {
    if (!parent || !child) return NX_XML_ERROR;

    // Detach from current parent if any
    if (child->parent) {
        nx_xml_node_remove_child(child->parent, child);
    }

    child->parent = parent;
    child->next_sibling = NULL;
    child->prev_sibling = parent->last_child;

    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
    parent->child_count++;
    return NX_XML_OK;
}

NxXmlError nx_xml_node_insert_before(NxXmlNode* parent, NxXmlNode* child, NxXmlNode* ref) {
    if (!parent || !child) return NX_XML_ERROR;
    if (!ref) return nx_xml_node_append_child(parent, child);

    if (ref->parent != parent) return NX_XML_ERROR;

    // Detach from current parent
    if (child->parent) {
        nx_xml_node_remove_child(child->parent, child);
    }

    child->parent = parent;
    child->next_sibling = ref;
    child->prev_sibling = ref->prev_sibling;

    if (ref->prev_sibling) {
        ref->prev_sibling->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    ref->prev_sibling = child;
    parent->child_count++;
    return NX_XML_OK;
}

NxXmlError nx_xml_node_remove_child(NxXmlNode* parent, NxXmlNode* child) {
    if (!parent || !child || child->parent != parent) return NX_XML_ERROR;

    if (child->prev_sibling) {
        child->prev_sibling->next_sibling = child->next_sibling;
    } else {
        parent->first_child = child->next_sibling;
    }

    if (child->next_sibling) {
        child->next_sibling->prev_sibling = child->prev_sibling;
    } else {
        parent->last_child = child->prev_sibling;
    }

    child->parent = NULL;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    parent->child_count--;
    return NX_XML_OK;
}

NxXmlError nx_xml_node_replace_child(NxXmlNode* parent, NxXmlNode* new_child, NxXmlNode* old_child) {
    if (!parent || !new_child || !old_child) return NX_XML_ERROR;
    if (old_child->parent != parent) return NX_XML_ERROR;

    NxXmlError err = nx_xml_node_insert_before(parent, new_child, old_child);
    if (err != NX_XML_OK) return err;
    return nx_xml_node_remove_child(parent, old_child);
}

// ============================================================================
// Node Creation
// ============================================================================

NxXmlNode* nx_xml_node_create_element(const char* tag) {
    NxXmlNode* n = (NxXmlNode*)calloc(1, sizeof(NxXmlNode));
    if (!n) return NULL;
    n->type = NX_XML_ELEMENT;
    n->name = tag ? strdup(tag) : NULL;
    return n;
}

NxXmlNode* nx_xml_node_create_text(const char* text) {
    NxXmlNode* n = (NxXmlNode*)calloc(1, sizeof(NxXmlNode));
    if (!n) return NULL;
    n->type = NX_XML_TEXT;
    n->value = text ? strdup(text) : NULL;
    return n;
}

void nx_xml_node_free_tree(NxXmlNode* node) {
    if (!node) return;
    NxXmlNode* child = node->first_child;
    while (child) {
        NxXmlNode* next = child->next_sibling;
        nx_xml_node_free_tree(child);
        child = next;
    }
    free(node->name);
    free(node->value);
    for (size_t i = 0; i < node->attr_count; ++i) {
        free(node->attrs[i].name);
        free(node->attrs[i].value);
    }
    free(node->attrs);
    free(node);
}

// ============================================================================
// Node Value Mutation
// ============================================================================

NxXmlError nx_xml_node_set_value(NxXmlNode* node, const char* value) {
    if (!node) return NX_XML_ERROR;
    free(node->value);
    node->value = value ? strdup(value) : NULL;
    return NX_XML_OK;
}

NxXmlError nx_xml_node_set_name(NxXmlNode* node, const char* name) {
    if (!node || node->type != NX_XML_ELEMENT) return NX_XML_ERROR;
    free(node->name);
    node->name = name ? strdup(name) : NULL;
    return NX_XML_OK;
}
