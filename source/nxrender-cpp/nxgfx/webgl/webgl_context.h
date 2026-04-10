// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace NXRender {

// ==================================================================
// WebGL Rendering Context
// Maps WebGL 1.0 API to OpenGL ES 2.0 / desktop GL
// ==================================================================

// WebGL constants (subset matching GL ES 2.0)
namespace GL {
    constexpr uint32_t VERTEX_SHADER = 0x8B31;
    constexpr uint32_t FRAGMENT_SHADER = 0x8B30;
    constexpr uint32_t ARRAY_BUFFER = 0x8892;
    constexpr uint32_t ELEMENT_ARRAY_BUFFER = 0x8893;
    constexpr uint32_t STATIC_DRAW = 0x88E4;
    constexpr uint32_t DYNAMIC_DRAW = 0x88E8;
    constexpr uint32_t STREAM_DRAW = 0x88E0;
    constexpr uint32_t FLOAT = 0x1406;
    constexpr uint32_t UNSIGNED_BYTE = 0x1401;
    constexpr uint32_t UNSIGNED_SHORT = 0x1403;
    constexpr uint32_t UNSIGNED_INT = 0x1405;
    constexpr uint32_t TRIANGLES = 0x0004;
    constexpr uint32_t TRIANGLE_STRIP = 0x0005;
    constexpr uint32_t TRIANGLE_FAN = 0x0006;
    constexpr uint32_t LINES = 0x0001;
    constexpr uint32_t LINE_STRIP = 0x0003;
    constexpr uint32_t LINE_LOOP = 0x0002;
    constexpr uint32_t POINTS = 0x0000;
    constexpr uint32_t TEXTURE_2D = 0x0DE1;
    constexpr uint32_t TEXTURE_CUBE_MAP = 0x8513;
    constexpr uint32_t RGBA = 0x1908;
    constexpr uint32_t RGB = 0x1907;
    constexpr uint32_t LUMINANCE = 0x1909;
    constexpr uint32_t ALPHA = 0x1906;
    constexpr uint32_t NEAREST = 0x2600;
    constexpr uint32_t LINEAR = 0x2601;
    constexpr uint32_t FRAMEBUFFER = 0x8D40;
    constexpr uint32_t RENDERBUFFER = 0x8D41;
    constexpr uint32_t DEPTH_TEST = 0x0B71;
    constexpr uint32_t BLEND = 0x0BE2;
    constexpr uint32_t STENCIL_TEST = 0x0B90;
    constexpr uint32_t SCISSOR_TEST = 0x0C11;
    constexpr uint32_t CULL_FACE = 0x0B44;
}

struct WebGLShader {
    uint32_t id = 0;
    uint32_t type = 0;
    std::string source;
    bool compiled = false;
    std::string infoLog;
};

struct WebGLProgram {
    uint32_t id = 0;
    uint32_t vs = 0, fs = 0;
    bool linked = false;
    std::string infoLog;
    std::unordered_map<std::string, int> uniformLocations;
    std::unordered_map<std::string, int> attribLocations;
};

struct WebGLBuffer {
    uint32_t id = 0;
    uint32_t target = 0;
    size_t size = 0;
};

struct WebGLTexture {
    uint32_t id = 0;
    int width = 0, height = 0;
    uint32_t format = 0;
};

struct WebGLFramebuffer {
    uint32_t id = 0;
    uint32_t colorAttachment = 0;
    uint32_t depthAttachment = 0;
};

struct WebGLRenderbuffer {
    uint32_t id = 0;
    int width = 0, height = 0;
    uint32_t format = 0;
};

struct WebGLUniformLocation {
    int location = -1;
    uint32_t program = 0;
};

// ==================================================================
// WebGLRenderingContext
// ==================================================================

class WebGLRenderingContext {
public:
    WebGLRenderingContext(int width, int height);
    ~WebGLRenderingContext();

    int drawingBufferWidth() const { return width_; }
    int drawingBufferHeight() const { return height_; }

    // Shader
    WebGLShader createShader(uint32_t type);
    void shaderSource(WebGLShader& shader, const std::string& source);
    void compileShader(WebGLShader& shader);
    std::string getShaderInfoLog(const WebGLShader& shader);
    bool getShaderParameter(const WebGLShader& shader, uint32_t pname);
    void deleteShader(WebGLShader& shader);

    // Program
    WebGLProgram createProgram();
    void attachShader(WebGLProgram& program, const WebGLShader& shader);
    void linkProgram(WebGLProgram& program);
    void useProgram(const WebGLProgram& program);
    std::string getProgramInfoLog(const WebGLProgram& program);
    bool getProgramParameter(const WebGLProgram& program, uint32_t pname);
    void deleteProgram(WebGLProgram& program);
    void detachShader(WebGLProgram& program, const WebGLShader& shader);

    // Uniform
    WebGLUniformLocation getUniformLocation(const WebGLProgram& program, const std::string& name);
    void uniform1f(const WebGLUniformLocation& loc, float v);
    void uniform2f(const WebGLUniformLocation& loc, float v0, float v1);
    void uniform3f(const WebGLUniformLocation& loc, float v0, float v1, float v2);
    void uniform4f(const WebGLUniformLocation& loc, float v0, float v1, float v2, float v3);
    void uniform1i(const WebGLUniformLocation& loc, int v);
    void uniformMatrix3fv(const WebGLUniformLocation& loc, bool transpose, const float* value);
    void uniformMatrix4fv(const WebGLUniformLocation& loc, bool transpose, const float* value);

    // Attribute
    int getAttribLocation(const WebGLProgram& program, const std::string& name);
    void enableVertexAttribArray(int index);
    void disableVertexAttribArray(int index);
    void vertexAttribPointer(int index, int size, uint32_t type,
                             bool normalized, int stride, size_t offset);

    // Buffer
    WebGLBuffer createBuffer();
    void bindBuffer(uint32_t target, const WebGLBuffer& buffer);
    void bufferData(uint32_t target, const void* data, size_t size, uint32_t usage);
    void bufferSubData(uint32_t target, size_t offset, const void* data, size_t size);
    void deleteBuffer(WebGLBuffer& buffer);

    // Texture
    WebGLTexture createTexture();
    void bindTexture(uint32_t target, const WebGLTexture& texture);
    void texImage2D(uint32_t target, int level, uint32_t internalFormat,
                    int width, int height, int border,
                    uint32_t format, uint32_t type, const void* pixels);
    void texSubImage2D(uint32_t target, int level, int xoffset, int yoffset,
                       int width, int height, uint32_t format, uint32_t type, const void* pixels);
    void texParameteri(uint32_t target, uint32_t pname, int param);
    void generateMipmap(uint32_t target);
    void activeTexture(uint32_t textureUnit);
    void deleteTexture(WebGLTexture& texture);

    // Framebuffer
    WebGLFramebuffer createFramebuffer();
    void bindFramebuffer(uint32_t target, const WebGLFramebuffer& fbo);
    void framebufferTexture2D(uint32_t target, uint32_t attachment,
                              uint32_t texTarget, const WebGLTexture& texture, int level);
    void framebufferRenderbuffer(uint32_t target, uint32_t attachment,
                                 uint32_t rbTarget, const WebGLRenderbuffer& rb);
    uint32_t checkFramebufferStatus(uint32_t target);
    void deleteFramebuffer(WebGLFramebuffer& fbo);

    // Renderbuffer
    WebGLRenderbuffer createRenderbuffer();
    void bindRenderbuffer(uint32_t target, const WebGLRenderbuffer& rb);
    void renderbufferStorage(uint32_t target, uint32_t format, int width, int height);
    void deleteRenderbuffer(WebGLRenderbuffer& rb);

    // Drawing
    void drawArrays(uint32_t mode, int first, int count);
    void drawElements(uint32_t mode, int count, uint32_t type, size_t offset);

    // State
    void viewport(int x, int y, int width, int height);
    void scissor(int x, int y, int width, int height);
    void clearColor(float r, float g, float b, float a);
    void clearDepth(float depth);
    void clearStencil(int s);
    void clear(uint32_t mask);
    void enable(uint32_t cap);
    void disable(uint32_t cap);
    void blendFunc(uint32_t sfactor, uint32_t dfactor);
    void blendFuncSeparate(uint32_t srcRGB, uint32_t dstRGB, uint32_t srcAlpha, uint32_t dstAlpha);
    void blendEquation(uint32_t mode);
    void depthFunc(uint32_t func);
    void depthMask(bool flag);
    void stencilFunc(uint32_t func, int ref, uint32_t mask);
    void stencilOp(uint32_t fail, uint32_t zfail, uint32_t zpass);
    void colorMask(bool r, bool g, bool b, bool a);
    void cullFace(uint32_t mode);
    void frontFace(uint32_t mode);
    void lineWidth(float width);
    void polygonOffset(float factor, float units);
    void pixelStorei(uint32_t pname, int param);
    void readPixels(int x, int y, int width, int height,
                    uint32_t format, uint32_t type, void* pixels);

    // Error
    uint32_t getError();

    // Extensions
    std::vector<std::string> getSupportedExtensions();
    bool getExtension(const std::string& name);

    // Context
    bool isContextLost() const { return contextLost_; }
    void loseContext();
    void restoreContext();

private:
    int width_, height_;
    bool contextLost_ = false;
    uint32_t defaultFBO_ = 0;
    std::vector<std::string> extensions_;
};

} // namespace NXRender
