/**
 * @file webgl_context.cpp
 * @brief WebGL rendering context implementation
 */

#include "webcore/webgl_context.hpp"
#include <iostream>

#ifdef ZEPRA_HAS_OPENGL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>

// Function pointers for OpenGL 3.x functions that may not be in gl.h
static PFNGLGENBUFFERSPROC glGenBuffersPtr = nullptr;
static PFNGLBINDBUFFERPROC glBindBufferPtr = nullptr;
static PFNGLBUFFERDATAPROC glBufferDataPtr = nullptr;
static PFNGLBUFFERSUBDATAPROC glBufferSubDataPtr = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersPtr = nullptr;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffersPtr = nullptr;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebufferPtr = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2DPtr = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffersPtr = nullptr;
static PFNGLUNIFORM1IPROC glUniform1iPtr = nullptr;
static PFNGLUNIFORM1FPROC glUniform1fPtr = nullptr;
static PFNGLUNIFORM2FPROC glUniform2fPtr = nullptr;
static PFNGLUNIFORM3FPROC glUniform3fPtr = nullptr;
static PFNGLUNIFORM4FPROC glUniform4fPtr = nullptr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fvPtr = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationPtr = nullptr;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocationPtr = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointerPtr = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArrayPtr = nullptr;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArrayPtr = nullptr;
static PFNGLCREATESHADERPROC glCreateShaderPtr = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSourcePtr = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShaderPtr = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderivPtr = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogPtr = nullptr;
static PFNGLDELETESHADERPROC glDeleteShaderPtr = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgramPtr = nullptr;
static PFNGLATTACHSHADERPROC glAttachShaderPtr = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgramPtr = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramivPtr = nullptr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLogPtr = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgramPtr = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramPtr = nullptr;
static PFNGLACTIVETEXTUREPROC glActiveTexturePtr = nullptr;

static bool extensionsLoaded = false;

static void loadGLExtensions() {
    if (extensionsLoaded) return;
    
    glGenBuffersPtr = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
    glBindBufferPtr = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
    glBufferDataPtr = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
    glBufferSubDataPtr = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
    glDeleteBuffersPtr = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
    glGenFramebuffersPtr = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
    glBindFramebufferPtr = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
    glFramebufferTexture2DPtr = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    glDeleteFramebuffersPtr = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    glUniform1iPtr = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
    glUniform1fPtr = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
    glUniform2fPtr = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
    glUniform3fPtr = (PFNGLUNIFORM3FPROC)SDL_GL_GetProcAddress("glUniform3f");
    glUniform4fPtr = (PFNGLUNIFORM4FPROC)SDL_GL_GetProcAddress("glUniform4f");
    glUniformMatrix4fvPtr = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
    glGetUniformLocationPtr = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
    glGetAttribLocationPtr = (PFNGLGETATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glGetAttribLocation");
    glVertexAttribPointerPtr = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArrayPtr = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
    glDisableVertexAttribArrayPtr = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glDisableVertexAttribArray");
    glCreateShaderPtr = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
    glShaderSourcePtr = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
    glCompileShaderPtr = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
    glGetShaderivPtr = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
    glGetShaderInfoLogPtr = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
    glDeleteShaderPtr = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
    glCreateProgramPtr = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
    glAttachShaderPtr = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
    glLinkProgramPtr = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
    glGetProgramivPtr = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
    glGetProgramInfoLogPtr = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
    glUseProgramPtr = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
    glDeleteProgramPtr = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
    glActiveTexturePtr = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
    
    extensionsLoaded = true;
}
#endif

namespace Zepra::WebCore {

// =============================================================================
// WebGLRenderingContext Implementation
// =============================================================================

WebGLRenderingContext::WebGLRenderingContext() = default;

WebGLRenderingContext::~WebGLRenderingContext() = default;

bool WebGLRenderingContext::initialize(int width, int height) {
#ifdef ZEPRA_HAS_OPENGL
    loadGLExtensions();
    
    width_ = width;
    height_ = height;
    initialized_ = true;
    
    // Set initial OpenGL state
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
#else
    (void)width;
    (void)height;
    std::cerr << "WebGL: OpenGL not available" << std::endl;
    return false;
#endif
}

void WebGLRenderingContext::resize(int width, int height) {
    width_ = width;
    height_ = height;
#ifdef ZEPRA_HAS_OPENGL
    glViewport(0, 0, width, height);
#endif
}

// -----------------------------------------------------------------------------
// Viewport and Clear
// -----------------------------------------------------------------------------

void WebGLRenderingContext::viewport(GLint x, GLint y, GLsizei w, GLsizei h) {
#ifdef ZEPRA_HAS_OPENGL
    glViewport(x, y, w, h);
#else
    (void)x; (void)y; (void)w; (void)h;
#endif
}

void WebGLRenderingContext::clearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
#ifdef ZEPRA_HAS_OPENGL
    glClearColor(r, g, b, a);
#else
    (void)r; (void)g; (void)b; (void)a;
#endif
}

void WebGLRenderingContext::clearDepth(GLclampf depth) {
#ifdef ZEPRA_HAS_OPENGL
    glClearDepth(depth);
#else
    (void)depth;
#endif
}

void WebGLRenderingContext::clear(GLbitfield mask) {
#ifdef ZEPRA_HAS_OPENGL
    glClear(mask);
#else
    (void)mask;
#endif
}

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

void WebGLRenderingContext::enable(GLenum cap) {
#ifdef ZEPRA_HAS_OPENGL
    glEnable(cap);
#else
    (void)cap;
#endif
}

void WebGLRenderingContext::disable(GLenum cap) {
#ifdef ZEPRA_HAS_OPENGL
    glDisable(cap);
#else
    (void)cap;
#endif
}

void WebGLRenderingContext::blendFunc(GLenum sfactor, GLenum dfactor) {
#ifdef ZEPRA_HAS_OPENGL
    glBlendFunc(sfactor, dfactor);
#else
    (void)sfactor; (void)dfactor;
#endif
}

void WebGLRenderingContext::depthFunc(GLenum func) {
#ifdef ZEPRA_HAS_OPENGL
    glDepthFunc(func);
#else
    (void)func;
#endif
}

void WebGLRenderingContext::cullFace(GLenum mode) {
#ifdef ZEPRA_HAS_OPENGL
    glCullFace(mode);
#else
    (void)mode;
#endif
}

// -----------------------------------------------------------------------------
// Shaders
// -----------------------------------------------------------------------------

WebGLShader WebGLRenderingContext::createShader(GLenum type) {
#ifdef ZEPRA_HAS_OPENGL
    return {glCreateShaderPtr(type)};
#else
    (void)type;
    return {};
#endif
}

void WebGLRenderingContext::shaderSource(WebGLShader shader, const std::string& source) {
#ifdef ZEPRA_HAS_OPENGL
    const char* src = source.c_str();
    glShaderSourcePtr(shader.id, 1, &src, nullptr);
#else
    (void)shader; (void)source;
#endif
}

void WebGLRenderingContext::compileShader(WebGLShader shader) {
#ifdef ZEPRA_HAS_OPENGL
    glCompileShaderPtr(shader.id);
#else
    (void)shader;
#endif
}

GLboolean WebGLRenderingContext::getShaderParameter(WebGLShader shader, GLenum pname) {
#ifdef ZEPRA_HAS_OPENGL
    GLint result;
    glGetShaderivPtr(shader.id, pname, &result);
    return result != 0;
#else
    (void)shader; (void)pname;
    return 0;
#endif
}

std::string WebGLRenderingContext::getShaderInfoLog(WebGLShader shader) {
#ifdef ZEPRA_HAS_OPENGL
    GLint length;
    glGetShaderivPtr(shader.id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::string log(length, '\0');
        glGetShaderInfoLogPtr(shader.id, length, nullptr, &log[0]);
        return log;
    }
#else
    (void)shader;
#endif
    return "";
}

void WebGLRenderingContext::deleteShader(WebGLShader shader) {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteShaderPtr(shader.id);
#else
    (void)shader;
#endif
}

// -----------------------------------------------------------------------------
// Programs
// -----------------------------------------------------------------------------

WebGLProgram WebGLRenderingContext::createProgram() {
#ifdef ZEPRA_HAS_OPENGL
    return {glCreateProgramPtr()};
#else
    return {};
#endif
}

void WebGLRenderingContext::attachShader(WebGLProgram program, WebGLShader shader) {
#ifdef ZEPRA_HAS_OPENGL
    glAttachShaderPtr(program.id, shader.id);
#else
    (void)program; (void)shader;
#endif
}

void WebGLRenderingContext::linkProgram(WebGLProgram program) {
#ifdef ZEPRA_HAS_OPENGL
    glLinkProgramPtr(program.id);
#else
    (void)program;
#endif
}

GLboolean WebGLRenderingContext::getProgramParameter(WebGLProgram program, GLenum pname) {
#ifdef ZEPRA_HAS_OPENGL
    GLint result;
    glGetProgramivPtr(program.id, pname, &result);
    return result != 0;
#else
    (void)program; (void)pname;
    return 0;
#endif
}

std::string WebGLRenderingContext::getProgramInfoLog(WebGLProgram program) {
#ifdef ZEPRA_HAS_OPENGL
    GLint length;
    glGetProgramivPtr(program.id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::string log(length, '\0');
        glGetProgramInfoLogPtr(program.id, length, nullptr, &log[0]);
        return log;
    }
#else
    (void)program;
#endif
    return "";
}

void WebGLRenderingContext::useProgram(WebGLProgram program) {
#ifdef ZEPRA_HAS_OPENGL
    glUseProgramPtr(program.id);
    currentProgram_ = program;
#else
    (void)program;
#endif
}

void WebGLRenderingContext::deleteProgram(WebGLProgram program) {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteProgramPtr(program.id);
#else
    (void)program;
#endif
}

// -----------------------------------------------------------------------------
// Attributes
// -----------------------------------------------------------------------------

GLint WebGLRenderingContext::getAttribLocation(WebGLProgram program, const std::string& name) {
#ifdef ZEPRA_HAS_OPENGL
    return glGetAttribLocationPtr(program.id, name.c_str());
#else
    (void)program; (void)name;
    return -1;
#endif
}

void WebGLRenderingContext::vertexAttribPointer(GLuint index, GLint size, GLenum type,
                                                 GLboolean normalized, GLsizei stride, 
                                                 GLintptr offset) {
#ifdef ZEPRA_HAS_OPENGL
    glVertexAttribPointerPtr(index, size, type, normalized, stride, 
                          reinterpret_cast<void*>(offset));
#else
    (void)index; (void)size; (void)type; (void)normalized; (void)stride; (void)offset;
#endif
}

void WebGLRenderingContext::enableVertexAttribArray(GLuint index) {
#ifdef ZEPRA_HAS_OPENGL
    glEnableVertexAttribArrayPtr(index);
#else
    (void)index;
#endif
}

void WebGLRenderingContext::disableVertexAttribArray(GLuint index) {
#ifdef ZEPRA_HAS_OPENGL
    glDisableVertexAttribArrayPtr(index);
#else
    (void)index;
#endif
}

// -----------------------------------------------------------------------------
// Uniforms
// -----------------------------------------------------------------------------

WebGLUniformLocation WebGLRenderingContext::getUniformLocation(WebGLProgram program, 
                                                                const std::string& name) {
#ifdef ZEPRA_HAS_OPENGL
    return {glGetUniformLocationPtr(program.id, name.c_str())};
#else
    (void)program; (void)name;
    return {};
#endif
}

void WebGLRenderingContext::uniform1i(WebGLUniformLocation loc, GLint v) {
#ifdef ZEPRA_HAS_OPENGL
    glUniform1iPtr(loc.loc, v);
#else
    (void)loc; (void)v;
#endif
}

void WebGLRenderingContext::uniform1f(WebGLUniformLocation loc, GLfloat v) {
#ifdef ZEPRA_HAS_OPENGL
    glUniform1fPtr(loc.loc, v);
#else
    (void)loc; (void)v;
#endif
}

void WebGLRenderingContext::uniform2f(WebGLUniformLocation loc, GLfloat x, GLfloat y) {
#ifdef ZEPRA_HAS_OPENGL
    glUniform2fPtr(loc.loc, x, y);
#else
    (void)loc; (void)x; (void)y;
#endif
}

void WebGLRenderingContext::uniform3f(WebGLUniformLocation loc, GLfloat x, GLfloat y, GLfloat z) {
#ifdef ZEPRA_HAS_OPENGL
    glUniform3fPtr(loc.loc, x, y, z);
#else
    (void)loc; (void)x; (void)y; (void)z;
#endif
}

void WebGLRenderingContext::uniform4f(WebGLUniformLocation loc, GLfloat x, GLfloat y, 
                                       GLfloat z, GLfloat w) {
#ifdef ZEPRA_HAS_OPENGL
    glUniform4fPtr(loc.loc, x, y, z, w);
#else
    (void)loc; (void)x; (void)y; (void)z; (void)w;
#endif
}

void WebGLRenderingContext::uniformMatrix4fv(WebGLUniformLocation loc, GLboolean transpose,
                                              const GLfloat* value) {
#ifdef ZEPRA_HAS_OPENGL
    glUniformMatrix4fvPtr(loc.loc, 1, transpose, value);
#else
    (void)loc; (void)transpose; (void)value;
#endif
}

// -----------------------------------------------------------------------------
// Buffers
// -----------------------------------------------------------------------------

WebGLBuffer WebGLRenderingContext::createBuffer() {
#ifdef ZEPRA_HAS_OPENGL
    GLuint id;
    glGenBuffersPtr(1, &id);
    return {id};
#else
    return {};
#endif
}

void WebGLRenderingContext::bindBuffer(GLenum target, WebGLBuffer buffer) {
#ifdef ZEPRA_HAS_OPENGL
    glBindBufferPtr(target, buffer.id);
    if (target == WebGLConstants::ARRAY_BUFFER) {
        currentArrayBuffer_ = buffer;
    } else if (target == WebGLConstants::ELEMENT_ARRAY_BUFFER) {
        currentElementBuffer_ = buffer;
    }
#else
    (void)target; (void)buffer;
#endif
}

void WebGLRenderingContext::bufferData(GLenum target, const void* data, 
                                        GLsizeiptr size, GLenum usage) {
#ifdef ZEPRA_HAS_OPENGL
    glBufferDataPtr(target, size, data, usage);
#else
    (void)target; (void)data; (void)size; (void)usage;
#endif
}

void WebGLRenderingContext::bufferSubData(GLenum target, GLintptr offset,
                                           const void* data, GLsizeiptr size) {
#ifdef ZEPRA_HAS_OPENGL
    glBufferSubDataPtr(target, offset, size, data);
#else
    (void)target; (void)offset; (void)data; (void)size;
#endif
}

void WebGLRenderingContext::deleteBuffer(WebGLBuffer buffer) {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteBuffersPtr(1, &buffer.id);
#else
    (void)buffer;
#endif
}

// -----------------------------------------------------------------------------
// Textures
// -----------------------------------------------------------------------------

WebGLTexture WebGLRenderingContext::createTexture() {
#ifdef ZEPRA_HAS_OPENGL
    GLuint id;
    glGenTextures(1, &id);
    return {id};
#else
    return {};
#endif
}

void WebGLRenderingContext::bindTexture(GLenum target, WebGLTexture texture) {
#ifdef ZEPRA_HAS_OPENGL
    glBindTexture(target, texture.id);
    if (target == WebGLConstants::TEXTURE_2D) {
        currentTexture_ = texture;
    }
#else
    (void)target; (void)texture;
#endif
}

void WebGLRenderingContext::activeTexture(GLenum texture) {
#ifdef ZEPRA_HAS_OPENGL
    glActiveTexturePtr(texture);
#else
    (void)texture;
#endif
}

void WebGLRenderingContext::texImage2D(GLenum target, GLint level, GLenum internalformat,
                                        GLsizei width, GLsizei height, GLint border,
                                        GLenum format, GLenum type, const void* pixels) {
#ifdef ZEPRA_HAS_OPENGL
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
#else
    (void)target; (void)level; (void)internalformat; (void)width; (void)height;
    (void)border; (void)format; (void)type; (void)pixels;
#endif
}

void WebGLRenderingContext::texParameteri(GLenum target, GLenum pname, GLint param) {
#ifdef ZEPRA_HAS_OPENGL
    glTexParameteri(target, pname, param);
#else
    (void)target; (void)pname; (void)param;
#endif
}

void WebGLRenderingContext::deleteTexture(WebGLTexture texture) {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteTextures(1, &texture.id);
#else
    (void)texture;
#endif
}

// -----------------------------------------------------------------------------
// Drawing
// -----------------------------------------------------------------------------

void WebGLRenderingContext::drawArrays(GLenum mode, GLint first, GLsizei count) {
#ifdef ZEPRA_HAS_OPENGL
    glDrawArrays(mode, first, count);
#else
    (void)mode; (void)first; (void)count;
#endif
}

void WebGLRenderingContext::drawElements(GLenum mode, GLsizei count, 
                                          GLenum type, GLintptr offset) {
#ifdef ZEPRA_HAS_OPENGL
    glDrawElements(mode, count, type, reinterpret_cast<void*>(offset));
#else
    (void)mode; (void)count; (void)type; (void)offset;
#endif
}

// -----------------------------------------------------------------------------
// Framebuffers
// -----------------------------------------------------------------------------

WebGLFramebuffer WebGLRenderingContext::createFramebuffer() {
#ifdef ZEPRA_HAS_OPENGL
    GLuint id;
    glGenFramebuffersPtr(1, &id);
    return {id};
#else
    return {};
#endif
}

void WebGLRenderingContext::bindFramebuffer(GLenum target, WebGLFramebuffer framebuffer) {
#ifdef ZEPRA_HAS_OPENGL
    glBindFramebufferPtr(target, framebuffer.id);
#else
    (void)target; (void)framebuffer;
#endif
}

void WebGLRenderingContext::framebufferTexture2D(GLenum target, GLenum attachment,
                                                  GLenum textarget, WebGLTexture texture,
                                                  GLint level) {
#ifdef ZEPRA_HAS_OPENGL
    glFramebufferTexture2DPtr(target, attachment, textarget, texture.id, level);
#else
    (void)target; (void)attachment; (void)textarget; (void)texture; (void)level;
#endif
}

void WebGLRenderingContext::deleteFramebuffer(WebGLFramebuffer framebuffer) {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteFramebuffersPtr(1, &framebuffer.id);
#else
    (void)framebuffer;
#endif
}

// -----------------------------------------------------------------------------
// Error Handling
// -----------------------------------------------------------------------------

GLenum WebGLRenderingContext::getError() {
#ifdef ZEPRA_HAS_OPENGL
    GLenum error = lastError_;
    lastError_ = WebGLConstants::NO_ERROR;
    if (error == WebGLConstants::NO_ERROR) {
        error = glGetError();
    }
    return error;
#else
    return WebGLConstants::NO_ERROR;
#endif
}

void WebGLRenderingContext::setError(GLenum error) {
    if (lastError_ == WebGLConstants::NO_ERROR) {
        lastError_ = error;
    }
}

} // namespace Zepra::WebCore
