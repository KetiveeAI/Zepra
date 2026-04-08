// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file shader.h
 * @brief OpenGL shader compilation, linking, and uniform management.
 *
 * Built-in shaders for all core rendering primitives are embedded as constexpr
 * string literals — no external file dependencies. Uniform values are cached
 * to avoid redundant glUniform calls.
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace NXRender {

/**
 * @brief Compiled OpenGL shader program.
 */
class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    ShaderProgram& operator=(ShaderProgram&&) noexcept;

    /**
     * @brief Compile and link from vertex/fragment source.
     * @return true on success.
     */
    bool compile(const char* vertexSrc, const char* fragmentSrc);

    /**
     * @brief Bind this shader for rendering.
     */
    void bind() const;

    /**
     * @brief Unbind (bind program 0).
     */
    static void unbind();

    /**
     * @brief Check if the program compiled and linked successfully.
     */
    bool isValid() const { return programId_ != 0; }

    /**
     * @brief Get the OpenGL program ID.
     */
    uint32_t programId() const { return programId_; }

    // ==================================================================
    // Uniform setters (cached — skips glUniform if value unchanged)
    // ==================================================================

    void setUniform1i(const char* name, int value);
    void setUniform1f(const char* name, float value);
    void setUniform2f(const char* name, float x, float y);
    void setUniform3f(const char* name, float x, float y, float z);
    void setUniform4f(const char* name, float x, float y, float z, float w);
    void setUniformMat4(const char* name, const float* matrix);

    /**
     * @brief Get uniform location (cached by name).
     */
    int getUniformLocation(const char* name);

    /**
     * @brief Get attribute location.
     */
    int getAttribLocation(const char* name);

    /**
     * @brief Retrieve the last compilation/link error message.
     */
    const std::string& errorLog() const { return errorLog_; }

private:
    uint32_t compileShader(uint32_t type, const char* source);
    void destroy();

    uint32_t programId_ = 0;
    std::string errorLog_;

    // Uniform location cache
    std::unordered_map<std::string, int> uniformCache_;

    // Uniform value cache for redundancy elimination
    struct UniformValue {
        union {
            int ival;
            float fval;
            float fval4[4];
            float mat4[16];
        };
        int type = 0; // 0=unset, 1=int, 2=float, 3=vec2, 4=vec3, 5=vec4, 6=mat4
    };
    std::unordered_map<int, UniformValue> uniformValueCache_;
};

/**
 * @brief Built-in shader library.
 *
 * Provides pre-compiled shaders for all core drawing operations.
 * Call initBuiltinShaders() at startup after GL context is created.
 */
namespace Shaders {

    /**
     * @brief Initialize all built-in shaders. Call after GL context creation.
     * @return true if all shaders compiled successfully.
     */
    bool initBuiltinShaders();

    /**
     * @brief Destroy all built-in shaders. Call before GL context destruction.
     */
    void destroyBuiltinShaders();

    /**
     * @brief Get the solid color shader (flat-colored geometry).
     */
    ShaderProgram* solidColor();

    /**
     * @brief Get the textured shader (texture + tint color).
     */
    ShaderProgram* textured();

    /**
     * @brief Get the SDF rounded rect shader.
     */
    ShaderProgram* roundedRect();

    /**
     * @brief Get the text SDF shader (glyph rendering).
     */
    ShaderProgram* textSDF();

    /**
     * @brief Get the linear gradient shader.
     */
    ShaderProgram* linearGradient();

    /**
     * @brief Get the radial gradient shader.
     */
    ShaderProgram* radialGradient();

    /**
     * @brief Get the box shadow (gaussian blur) shader.
     */
    ShaderProgram* boxShadow();

    // ==================================================================
    // Embedded shader sources
    // ==================================================================

    extern const char* kSolidColorVert;
    extern const char* kSolidColorFrag;
    extern const char* kTexturedVert;
    extern const char* kTexturedFrag;
    extern const char* kRoundedRectVert;
    extern const char* kRoundedRectFrag;
    extern const char* kTextSDFVert;
    extern const char* kTextSDFFrag;
    extern const char* kLinearGradientVert;
    extern const char* kLinearGradientFrag;
    extern const char* kRadialGradientVert;
    extern const char* kRadialGradientFrag;
    extern const char* kBoxShadowVert;
    extern const char* kBoxShadowFrag;

} // namespace Shaders

} // namespace NXRender
