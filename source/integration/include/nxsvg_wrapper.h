/**
 * NXSVG C Wrapper - Allows C programs (like IconLay) to use NXSVG
 * 
 * Links IconLay to GPU-accelerated SVG rendering from ZepraBrowser's NXSVG
 */

#ifndef NXSVG_WRAPPER_H
#define NXSVG_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handles */
typedef void* nxsvg_loader_t;
typedef void* nxsvg_image_t;

/* Group info structure (C-compatible) */
typedef struct {
    const char* id;
    const char* name;
    const char* class_name;
    float opacity;
    int visible;
    float transform[6];  /* a b c d e f */
    int has_transform;
} nxsvg_group_info_t;

/* Loader lifecycle */
nxsvg_loader_t nxsvg_loader_create(void);
void nxsvg_loader_destroy(nxsvg_loader_t loader);

/* Load SVG from file or string */
int nxsvg_load_file(nxsvg_loader_t loader, const char* name, const char* path);
int nxsvg_load_string(nxsvg_loader_t loader, const char* name, const char* svg_data);

/* Check if image exists */
int nxsvg_has(nxsvg_loader_t loader, const char* name);

/* Get image reference (for querying) */
nxsvg_image_t nxsvg_get_image(nxsvg_loader_t loader, const char* name);

/* Render entire SVG */
void nxsvg_render(nxsvg_loader_t loader, const char* name, 
                  float x, float y, float size,
                  uint8_t r, uint8_t g, uint8_t b);

/* Render specific group */
void nxsvg_render_group(nxsvg_loader_t loader, const char* name, const char* group_id,
                        float x, float y, float size,
                        uint8_t r, uint8_t g, uint8_t b);

/* Query group information */
size_t nxsvg_group_count(nxsvg_image_t img);
int nxsvg_get_group_info(nxsvg_image_t img, size_t index, nxsvg_group_info_t* info);
int nxsvg_get_group_by_id(nxsvg_image_t img, const char* id, nxsvg_group_info_t* info);

/* Get viewBox info */
void nxsvg_get_viewbox(nxsvg_image_t img, float* x, float* y, float* w, float* h);

/* Get shape count */
size_t nxsvg_shape_count(nxsvg_image_t img);

#ifdef __cplusplus
}
#endif

#endif /* NXSVG_WRAPPER_H */
