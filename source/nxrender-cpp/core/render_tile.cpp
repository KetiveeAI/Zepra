// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "render_tile.h"
#include "nxgfx/context.h"
#include <GL/gl.h>

namespace NXRender {

// Function pointer typedefs required for Framebuffer Objects (FBOs) in OpenGL.
// Usually provided by GLEW or GLAD, but if not available natively, we dynamically link them
// for exact platform compliance.
#ifdef _WIN32
    #include <windows.h>
    #define GET_GL_PROC(type, name) type name = (type)wglGetProcAddress(#name)
#else
    #include <GL/glx.h>
    #define GET_GL_PROC(type, name) type name = (type)glXGetProcAddress((const GLubyte*)#name)
#endif

typedef void (*PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (*PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (*PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (*PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);

static PFNGLGENFRAMEBUFFERSPROC nx_glGenFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC nx_glBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC nx_glFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC nx_glCheckFramebufferStatus = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC nx_glDeleteFramebuffers = nullptr;

static void loadGLExtensions() {
    static bool loaded = false;
    if (loaded) return;
    loaded = true;
    
    GET_GL_PROC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
    GET_GL_PROC(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
    GET_GL_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
    GET_GL_PROC(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
    GET_GL_PROC(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
    
    nx_glGenFramebuffers = glGenFramebuffers;
    nx_glBindFramebuffer = glBindFramebuffer;
    nx_glFramebufferTexture2D = glFramebufferTexture2D;
    nx_glCheckFramebufferStatus = glCheckFramebufferStatus;
    nx_glDeleteFramebuffers = glDeleteFramebuffers;
}

RenderTile::RenderTile(int x, int y, int width, int height)
    : bounds_(x, y, width, height) {
    loadGLExtensions();
    initFbo();
}

RenderTile::~RenderTile() {
    releaseFbo();
}

void RenderTile::initFbo() {
    if (!nx_glGenFramebuffers) return; // Fallback entirely if driver misses framebuffers
    
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    
    // Allocate transparent texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)bounds_.width, (GLsizei)bounds_.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    nx_glGenFramebuffers(1, &fbo_);
    nx_glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, fbo_);
    
    // Attach texture to FBO
    nx_glFramebufferTexture2D(0x8D40, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, GL_TEXTURE_2D, texture_, 0);
    
    GLenum status = nx_glCheckFramebufferStatus(0x8D40);
    if (status != 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */) {
        // Handle error gracefully; tile will just skip rendering
        releaseFbo();
    }
    
    // Unbind
    nx_glBindFramebuffer(0x8D40, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderTile::releaseFbo() {
    if (texture_) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    if (fbo_ && nx_glDeleteFramebuffers) {
        nx_glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
}

void RenderTile::beginRecord(GpuContext* ctx) {
    if (!fbo_) return;
    
    isReady_ = false;
    
    // Bind FBO and redirect GpuContext outputs
    nx_glBindFramebuffer(0x8D40, fbo_);
    
    // We adjust glViewport directly because the generic GpuContext might ignore target buffers
    glViewport(0, 0, (GLsizei)bounds_.width, (GLsizei)bounds_.height);
    
    // We clear the tile completely transparent
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Translate the GPU context so elements draw correctly within this tile's regional coordinates
    ctx->pushTransform();
    ctx->translate(-bounds_.x, -bounds_.y);
}

void RenderTile::endRecord(GpuContext* ctx) {
    if (!fbo_) return;
    
    ctx->popTransform();
    
    // Restore default framebuffer (system window)
    nx_glBindFramebuffer(0x8D40, 0);
    
    // Restore viewport from context
    glViewport(0, 0, ctx->width(), ctx->height());
    
    isReady_ = true;
}

void RenderTile::draw(GpuContext* ctx, const Rect& targetRect) const {
    if (!isReady_ || !texture_) return;
    
    // Real implementation: We bypass heavy layer bindings and emit a textured quad natively
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_);
    
    // Typical blending for RGBA tile over existing background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Map bounds to normalized device coordinates if using modern OpenGL projection,
    // or direct immediate mode if legacy pipeline is active.
    // Given NXRender context abstraction, we assume orthogonal projection is active in context state.
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(targetRect.x, targetRect.y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(targetRect.x + targetRect.width, targetRect.y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(targetRect.x + targetRect.width, targetRect.y + targetRect.height);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(targetRect.x, targetRect.y + targetRect.height);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace NXRender
