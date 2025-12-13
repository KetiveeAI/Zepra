/**
 * @file gl_render_backend.cpp
 * @brief OpenGL GPU render backend implementation
 */

#include "webcore/gl_render_backend.hpp"
#include <cstring>
#include <cmath>
#include <iostream>

#ifdef ZEPRA_HAS_OPENGL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

// Function pointers for OpenGL 3.x functions
static PFNGLGENVERTEXARRAYSPROC glGenVertexArraysPtr = nullptr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArrayPtr = nullptr;
static PFNGLGENBUFFERSPROC glGenBuffersPtr = nullptr;
static PFNGLBINDBUFFERPROC glBindBufferPtr = nullptr;
static PFNGLBUFFERDATAPROC glBufferDataPtr = nullptr;
static PFNGLBUFFERSUBDATAPROC glBufferSubDataPtr = nullptr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointerPtr = nullptr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArrayPtr = nullptr;
static PFNGLCREATESHADERPROC glCreateShaderPtr = nullptr;
static PFNGLSHADERSOURCEPROC glShaderSourcePtr = nullptr;
static PFNGLCOMPILESHADERPROC glCompileShaderPtr = nullptr;
static PFNGLGETSHADERIVPROC glGetShaderivPtr = nullptr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogPtr = nullptr;
static PFNGLCREATEPROGRAMPROC glCreateProgramPtr = nullptr;
static PFNGLATTACHSHADERPROC glAttachShaderPtr = nullptr;
static PFNGLLINKPROGRAMPROC glLinkProgramPtr = nullptr;
static PFNGLGETPROGRAMIVPROC glGetProgramivPtr = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgramPtr = nullptr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationPtr = nullptr;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocationPtr = nullptr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fvPtr = nullptr;
static PFNGLUNIFORM4FPROC glUniform4fPtr = nullptr;
static PFNGLUNIFORM1IPROC glUniform1iPtr = nullptr;
static PFNGLDELETESHADERPROC glDeleteShaderPtr = nullptr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramPtr = nullptr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersPtr = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArraysPtr = nullptr;

static bool glFunctionsLoaded = false;

static void loadGLFunctions() {
    if (glFunctionsLoaded) return;
    
    glGenVertexArraysPtr = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
    glBindVertexArrayPtr = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
    glGenBuffersPtr = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
    glBindBufferPtr = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
    glBufferDataPtr = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
    glBufferSubDataPtr = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
    glVertexAttribPointerPtr = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArrayPtr = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
    glCreateShaderPtr = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
    glShaderSourcePtr = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
    glCompileShaderPtr = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
    glGetShaderivPtr = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
    glGetShaderInfoLogPtr = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
    glCreateProgramPtr = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
    glAttachShaderPtr = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
    glLinkProgramPtr = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
    glGetProgramivPtr = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
    glUseProgramPtr = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
    glGetUniformLocationPtr = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
    glGetAttribLocationPtr = (PFNGLGETATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glGetAttribLocation");
    glUniformMatrix4fvPtr = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
    glUniform4fPtr = (PFNGLUNIFORM4FPROC)SDL_GL_GetProcAddress("glUniform4f");
    glUniform1iPtr = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
    glDeleteShaderPtr = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
    glDeleteProgramPtr = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
    glDeleteBuffersPtr = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
    glDeleteVertexArraysPtr = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays");
    
    glFunctionsLoaded = true;
    std::cout << "OpenGL extension functions loaded successfully" << std::endl;
}

#else
// No OpenGL - provide stub types
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef int GLsizei;
typedef char GLchar;
typedef unsigned char GLboolean;
#endif

namespace Zepra::WebCore {

// Vertex shader for solid color rendering
static const char* SOLID_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vColor;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* SOLID_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

// Vertex shader for textured rendering
static const char* TEXTURE_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 vTexCoord;
out vec4 vColor;
uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

static const char* TEXTURE_FRAGMENT_SHADER = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
out vec4 FragColor;
uniform sampler2D uTexture;

void main() {
    FragColor = texture(uTexture, vTexCoord) * vColor;
}
)";

// =============================================================================
// GLRenderBackend
// =============================================================================

GLRenderBackend::GLRenderBackend() {
    vertices_.reserve(MAX_QUADS * 6 * 6); // 6 vertices per quad, 6 floats per vertex
    indices_.reserve(MAX_QUADS * 6);
}

GLRenderBackend::~GLRenderBackend() {
    shutdown();
}

bool GLRenderBackend::initialize() {
    if (initialized_) return true;
    
#ifdef ZEPRA_HAS_OPENGL
    loadGLFunctions();
    
    if (!glCreateShaderPtr || !glCreateProgramPtr) {
        std::cerr << "Failed to load OpenGL extension functions" << std::endl;
        return false;
    }
#endif
    
    if (!createShaders()) return false;
    createBuffers();
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
    } else {
        // Load default system font
        font_ = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
        if (!font_) {
             std::cerr << "Failed to load default font. Trying alternative..." << std::endl;
             font_ = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 14);
        }
        if (!font_) {
             std::cerr << "Failed to render text: No system font found!" << std::endl;
        }
    }
    
    initialized_ = true;
    return true;
}

void GLRenderBackend::shutdown() {
    if (!initialized_) return;
    
    destroyShaders();
    destroyBuffers();
    
    // Cleanup texture cache
    for (auto& [path, tex] : textureCache_) {
        glDeleteTextures(1, &tex.id);
    }
    textureCache_.clear();
    
    textureCache_.clear();
    
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }

    TTF_Quit();
    IMG_Quit();
    
    initialized_ = false;
}

bool GLRenderBackend::createShaders() {
#ifdef ZEPRA_HAS_OPENGL
    // Create solid shader
    GLuint vs = glCreateShaderPtr(GL_VERTEX_SHADER);
    glShaderSourcePtr(vs, 1, &SOLID_VERTEX_SHADER, nullptr);
    glCompileShaderPtr(vs);
    
    GLint success;
    glGetShaderivPtr(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLogPtr(vs, 512, nullptr, log);
        std::cerr << "Vertex shader error: " << log << std::endl;
        return false;
    }
    
    GLuint fs = glCreateShaderPtr(GL_FRAGMENT_SHADER);
    glShaderSourcePtr(fs, 1, &SOLID_FRAGMENT_SHADER, nullptr);
    glCompileShaderPtr(fs);
    
    glGetShaderivPtr(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLogPtr(fs, 512, nullptr, log);
        std::cerr << "Fragment shader error: " << log << std::endl;
        glDeleteShaderPtr(vs);
        return false;
    }
    
    solidShader_.id = glCreateProgramPtr();
    glAttachShaderPtr(solidShader_.id, vs);
    glAttachShaderPtr(solidShader_.id, fs);
    glLinkProgramPtr(solidShader_.id);
    
    glGetProgramivPtr(solidShader_.id, GL_LINK_STATUS, &success);
    glDeleteShaderPtr(vs);
    glDeleteShaderPtr(fs);
    
    if (!success) {
        std::cerr << "Shader program link error" << std::endl;
        return false;
    }
    
    solidShader_.posLocation = glGetAttribLocationPtr(solidShader_.id, "aPos");
    solidShader_.colorLocation = glGetAttribLocationPtr(solidShader_.id, "aColor");
    solidShader_.projectionLocation = glGetUniformLocationPtr(solidShader_.id, "uProjection");
    
    std::cout << "OpenGL solid shader compiled successfully" << std::endl;
    
    // Create texture shader
    GLuint tvs = glCreateShaderPtr(GL_VERTEX_SHADER);
    glShaderSourcePtr(tvs, 1, &TEXTURE_VERTEX_SHADER, nullptr);
    glCompileShaderPtr(tvs);
    
    glGetShaderivPtr(tvs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLogPtr(tvs, 512, nullptr, log);
        std::cerr << "Texture Vertex shader error: " << log << std::endl;
        return false;
    }
    
    GLuint tfs = glCreateShaderPtr(GL_FRAGMENT_SHADER);
    glShaderSourcePtr(tfs, 1, &TEXTURE_FRAGMENT_SHADER, nullptr);
    glCompileShaderPtr(tfs);
    
    glGetShaderivPtr(tfs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLogPtr(tfs, 512, nullptr, log);
        std::cerr << "Texture Fragment shader error: " << log << std::endl;
        glDeleteShaderPtr(tvs);
        return false;
    }
    
    textureShader_.id = glCreateProgramPtr();
    glAttachShaderPtr(textureShader_.id, tvs);
    glAttachShaderPtr(textureShader_.id, tfs);
    glLinkProgramPtr(textureShader_.id);
    
    glGetProgramivPtr(textureShader_.id, GL_LINK_STATUS, &success);
    glDeleteShaderPtr(tvs);
    glDeleteShaderPtr(tfs);
    
    if (!success) {
        std::cerr << "Texture shader program link error" << std::endl;
        return false;
    }
    
    textureShader_.posLocation = glGetAttribLocationPtr(textureShader_.id, "aPos");
    textureShader_.texCoordLocation = glGetAttribLocationPtr(textureShader_.id, "aTexCoord");
    textureShader_.colorLocation = glGetAttribLocationPtr(textureShader_.id, "aColor");
    textureShader_.projectionLocation = glGetUniformLocationPtr(textureShader_.id, "uProjection");
    textureShader_.textureLocation = glGetUniformLocationPtr(textureShader_.id, "uTexture");
    
    std::cout << "OpenGL shaders compiled and linked successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GLRenderBackend::destroyShaders() {
#ifdef ZEPRA_HAS_OPENGL
    if (solidShader_.id) glDeleteProgramPtr(solidShader_.id);
    if (textureShader_.id) glDeleteProgramPtr(textureShader_.id);
#endif
    solidShader_ = {};
    textureShader_ = {};
}

void GLRenderBackend::createBuffers() {
#ifdef ZEPRA_HAS_OPENGL
    glGenVertexArraysPtr(1, &vao_);
    glGenBuffersPtr(1, &vbo_);
    glGenBuffersPtr(1, &ebo_);
    
    glBindVertexArrayPtr(vao_);
    
    glBindBufferPtr(GL_ARRAY_BUFFER, vbo_);
    glBufferDataPtr(GL_ARRAY_BUFFER, MAX_QUADS * 6 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    // Position (2 floats) + Color (4 floats)
    glVertexAttribPointerPtr(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArrayPtr(0);
    glVertexAttribPointerPtr(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArrayPtr(1);
    
    // Generate indices for quads (2 triangles per quad)
    std::vector<unsigned int> indices;
    indices.reserve(MAX_QUADS * 6);
    for (int i = 0; i < MAX_QUADS; i++) {
        int base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }
    
    glBindBufferPtr(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferDataPtr(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glBindVertexArrayPtr(0);
    
    // Create texture VAO/VBO
    glGenVertexArraysPtr(1, &textureVao_);
    glGenBuffersPtr(1, &textureVbo_);
    glGenBuffersPtr(1, &textureEbo_);
    
    glBindVertexArrayPtr(textureVao_);
    
    glBindBufferPtr(GL_ARRAY_BUFFER, textureVbo_);
    // x,y,u,v,r,g,b,a = 8 floats per vertex * 4 vertices = 32 floats
    glBufferDataPtr(GL_ARRAY_BUFFER, 32 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    // Position (2 floats)
    glVertexAttribPointerPtr(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArrayPtr(0);
    // TexCoord (2 floats)
    glVertexAttribPointerPtr(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArrayPtr(1);
    // Color (4 floats)
    glVertexAttribPointerPtr(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArrayPtr(2);
    
    unsigned int texIndices[] = { 0, 1, 2, 2, 3, 0 };
    glBindBufferPtr(GL_ELEMENT_ARRAY_BUFFER, textureEbo_);
    glBufferDataPtr(GL_ELEMENT_ARRAY_BUFFER, sizeof(texIndices), texIndices, GL_STATIC_DRAW);
    
    glBindVertexArrayPtr(0);

    std::cout << "OpenGL buffers created successfully" << std::endl;
#endif
}

void GLRenderBackend::destroyBuffers() {
#ifdef ZEPRA_HAS_OPENGL
    glDeleteBuffersPtr(1, &vbo_);
    glDeleteBuffersPtr(1, &ebo_);
    glDeleteVertexArraysPtr(1, &vao_);
    
    glDeleteBuffersPtr(1, &textureVbo_);
    glDeleteBuffersPtr(1, &textureEbo_);
    glDeleteVertexArraysPtr(1, &textureVao_);
#endif
    vbo_ = ebo_ = vao_ = 0;
}

void GLRenderBackend::addQuad(const Rect& rect, const Color& color, const Rect*) {
    if (quadCount_ >= MAX_QUADS) {
        flushBatch();
    }

    
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;
    
    // 4 vertices per quad
    // Top-left
    vertices_.push_back(rect.x);
    vertices_.push_back(rect.y);
    vertices_.push_back(r); vertices_.push_back(g); vertices_.push_back(b); vertices_.push_back(a);
    
    // Top-right
    vertices_.push_back(rect.x + rect.width);
    vertices_.push_back(rect.y);
    vertices_.push_back(r); vertices_.push_back(g); vertices_.push_back(b); vertices_.push_back(a);
    
    // Bottom-right
    vertices_.push_back(rect.x + rect.width);
    vertices_.push_back(rect.y + rect.height);
    vertices_.push_back(r); vertices_.push_back(g); vertices_.push_back(b); vertices_.push_back(a);
    
    // Bottom-left
    vertices_.push_back(rect.x);
    vertices_.push_back(rect.y + rect.height);
    vertices_.push_back(r); vertices_.push_back(g); vertices_.push_back(b); vertices_.push_back(a);
    
    quadCount_++;
}

void GLRenderBackend::flushBatch() {
    if (quadCount_ == 0) return;
    
#ifdef ZEPRA_HAS_OPENGL
    glUseProgramPtr(solidShader_.id);
    
    // Set orthographic projection
    float proj[16] = {
        2.0f / viewportWidth_, 0, 0, 0,
        0, -2.0f / viewportHeight_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fvPtr(solidShader_.projectionLocation, 1, GL_FALSE, proj);
    
    glBindVertexArrayPtr(vao_);
    glBindBufferPtr(GL_ARRAY_BUFFER, vbo_);
    glBufferSubDataPtr(GL_ARRAY_BUFFER, 0, vertices_.size() * sizeof(float), vertices_.data());
    
    glDrawElements(GL_TRIANGLES, quadCount_ * 6, GL_UNSIGNED_INT, nullptr);
#endif
    
    drawCallCount_++;
    triangleCount_ += quadCount_ * 2;

    
    vertices_.clear();
    quadCount_ = 0;
}

void GLRenderBackend::executeDisplayList(const DisplayList& list) {
    drawCallCount_ = 0;
    triangleCount_ = 0;
    
    for (const auto& cmd : list.commands()) {
        switch (cmd.type) {
            case PaintCommandType::FillRect:
                executeFillRect(cmd);
                break;
            case PaintCommandType::StrokeRect:
                executeStrokeRect(cmd);
                break;
            case PaintCommandType::DrawText:
                executeDrawText(cmd);
                break;
            case PaintCommandType::DrawImage:
                executeDrawImage(cmd);
                break;
            case PaintCommandType::DrawTexture:
                executeDrawTexture(cmd);
                break;
            case PaintCommandType::PushClip:
                executePushClip(cmd);
                break;
            case PaintCommandType::PopClip:
                executePopClip();
                break;
            default:
                break;
        }
    }
    
    flushBatch();
}

void GLRenderBackend::executeFillRect(const PaintCommand& cmd) {
    addQuad(cmd.rect, cmd.color);
}

void GLRenderBackend::executeStrokeRect(const PaintCommand& cmd) {
    float w = cmd.number > 0 ? cmd.number : 1.0f;
    const Rect& r = cmd.rect;
    const Color& c = cmd.color;
    
    // Top
    addQuad({r.x, r.y, r.width, w}, c);
    // Bottom
    addQuad({r.x, r.y + r.height - w, r.width, w}, c);
    // Left
    addQuad({r.x, r.y + w, w, r.height - 2 * w}, c);
    // Right
    addQuad({r.x + r.width - w, r.y + w, w, r.height - 2 * w}, c);
}

void GLRenderBackend::executeDrawText(const PaintCommand& cmd) {
#ifdef ZEPRA_HAS_OPENGL
    if (cmd.text.empty()) {
        return;
    }
    
    if (!font_) {
        // Font not loaded - this is the problem!
        // For now, skip text rendering rather than crash
        return;
    }

    // Cache key: text + size + color
    std::string key = "txt_" + cmd.text + "_" + std::to_string((int)cmd.number) + "_" + 
                      std::to_string(cmd.color.r) + "_" + std::to_string(cmd.color.g) + "_" + 
                      std::to_string(cmd.color.b);
    
    auto it = textureCache_.find(key);
    if (it == textureCache_.end()) {
        SDL_Color textColor = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};
        
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, cmd.text.c_str(), textColor);
        if (!surface) return;
        
        // Convert surface to RGBA32 format for OpenGL compatibility
        SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        if (!converted) return;
        
        // Upload
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        GLTexture glTex;
        glTex.id = tex;
        glTex.width = converted->w;
        glTex.height = converted->h;
        textureCache_[key] = glTex;
        
        SDL_FreeSurface(converted);
        it = textureCache_.find(key);
    }
    
    // Draw using texture logic
    flushBatch();
    
    // Enable blending for text transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgramPtr(textureShader_.id);

    float proj[16] = {
        2.0f / viewportWidth_, 0, 0, 0,
        0, -2.0f / viewportHeight_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fvPtr(textureShader_.projectionLocation, 1, GL_FALSE, proj);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    glUniform1iPtr(textureShader_.textureLocation, 0);

    // Draw quad - texture already has color baked in
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f; 
    
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = static_cast<float>(it->second.width);
    float h = static_cast<float>(it->second.height);

    float vertices[] = {
        x, y,         0.0f, 0.0f, r, g, b, a,
        x+w, y,       1.0f, 0.0f, r, g, b, a,
        x+w, y+h,     1.0f, 1.0f, r, g, b, a,
        x, y+h,       0.0f, 1.0f, r, g, b, a
    };
    
    glBindVertexArrayPtr(textureVao_);
    glBindBufferPtr(GL_ARRAY_BUFFER, textureVbo_);
    glBufferSubDataPtr(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArrayPtr(0);

#endif
}

void GLRenderBackend::executeDrawImage(const PaintCommand& cmd) {
    if (cmd.imagePath.empty()) return;

#ifdef ZEPRA_HAS_OPENGL
    // Check cache
    auto it = textureCache_.find(cmd.imagePath);
    if (it == textureCache_.end()) {
        // Load texture
        SDL_Surface* surface = IMG_Load(cmd.imagePath.c_str());
        if (!surface) {
            std::cerr << "Failed to load image: " << cmd.imagePath << " Error: " << IMG_GetError() << std::endl;
            return;
        }

        // Convert to RGBA
        SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        if (!converted) return;

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLTexture glTex;
        glTex.id = tex;
        glTex.width = converted->w;
        glTex.height = converted->h;
        textureCache_[cmd.imagePath] = glTex;

        SDL_FreeSurface(converted);
        it = textureCache_.find(cmd.imagePath);
    }

    // Flush any pending solid geometry
    flushBatch();

    // Setup for texture drawing (simplified: assuming one texture per batch for now or immediate mode style for textures)
    // For proper batching we'd need multiple texture units or texture arrays.
    // For now, let's just draw immediately for textures (inefficient but works)
   
    glUseProgramPtr(textureShader_.id);

    float proj[16] = {
        2.0f / viewportWidth_, 0, 0, 0,
        0, -2.0f / viewportHeight_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fvPtr(textureShader_.projectionLocation, 1, GL_FALSE, proj);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, it->second.id);
    glUniform1iPtr(textureShader_.textureLocation, 0);

    // Draw quad manually
    float r = cmd.color.r / 255.0f;
    float g = cmd.color.g / 255.0f;
    float b = cmd.color.b / 255.0f;
    float a = cmd.color.a / 255.0f;
    
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;

    // x, y, u, v, r, g, b, a
    float vertices[] = {
        x, y,         0.0f, 0.0f, r, g, b, a,
        x+w, y,       1.0f, 0.0f, r, g, b, a,
        x+w, y+h,     1.0f, 1.0f, r, g, b, a,
        x, y+h,       0.0f, 1.0f, r, g, b, a
    };
    
    glBindVertexArrayPtr(textureVao_);
    glBindBufferPtr(GL_ARRAY_BUFFER, textureVbo_);
    glBufferSubDataPtr(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArrayPtr(0);

#endif
}

void GLRenderBackend::executeDrawTexture(const PaintCommand& cmd) {
    if (cmd.textureId == 0) return;

#ifdef ZEPRA_HAS_OPENGL
    flushBatch();
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgramPtr(textureShader_.id);
    
    float proj[] = {
        2.0f / viewportWidth_, 0, 0, 0,
        0, -2.0f / viewportHeight_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fvPtr(textureShader_.projectionLocation, 1, GL_FALSE, proj);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd.textureId);
    glUniform1iPtr(textureShader_.textureLocation, 0);

    // Draw quad with full white color to show texture as-is
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;

    float vertices[] = {
        x, y,         0.0f, 0.0f, r, g, b, a,
        x+w, y,       1.0f, 0.0f, r, g, b, a,
        x+w, y+h,     1.0f, 1.0f, r, g, b, a,
        x, y+h,       0.0f, 1.0f, r, g, b, a
    };
    
    glBindVertexArrayPtr(textureVao_);
    glBindBufferPtr(GL_ARRAY_BUFFER, textureVbo_);
    glBufferSubDataPtr(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArrayPtr(0);

#endif
}

void GLRenderBackend::executePushClip(const PaintCommand& cmd) {
    flushBatch();
    clipStack_.push_back(cmd.rect);
    
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        static_cast<GLint>(cmd.rect.x),
        static_cast<GLint>(viewportHeight_ - cmd.rect.y - cmd.rect.height),
        static_cast<GLsizei>(cmd.rect.width),
        static_cast<GLsizei>(cmd.rect.height)
    );
}

void GLRenderBackend::executePopClip() {
    flushBatch();
    clipStack_.pop_back();
    
    if (clipStack_.empty()) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        const Rect& r = clipStack_.back();
        glScissor(
            static_cast<GLint>(r.x),
            static_cast<GLint>(viewportHeight_ - r.y - r.height),
            static_cast<GLsizei>(r.width),
            static_cast<GLsizei>(r.height)
        );
    }
}

void GLRenderBackend::present() {
    // Swap buffers - handled by platform layer
}

void GLRenderBackend::resize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    glViewport(0, 0, width, height);
}

void* GLRenderBackend::createTexture(int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(tex));
}

void GLRenderBackend::destroyTexture(void* texture) {
    GLuint tex = static_cast<GLuint>(reinterpret_cast<uintptr_t>(texture));
    glDeleteTextures(1, &tex);
}

void GLRenderBackend::setClearColor(const Color& color) {
    clearColor_ = color;
}

void GLRenderBackend::clear() {
    glClearColor(
        clearColor_.r / 255.0f,
        clearColor_.g / 255.0f,
        clearColor_.b / 255.0f,
        clearColor_.a / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderBackend::beginLayer(float opacity) {
    currentLayerOpacity_ = opacity;
    // Would create framebuffer for layer compositing
}

void GLRenderBackend::endLayer() {
    // Would composite layer back to main framebuffer with opacity
    currentLayerOpacity_ = 1.0f;
}

// =============================================================================
// SoftwareRenderBackend (CPU fallback)
// =============================================================================

SoftwareRenderBackend::SoftwareRenderBackend(int width, int height)
    : width_(width), height_(height) {
    pixels_.resize(width * height, 0xFFFFFFFF); // White background
}

SoftwareRenderBackend::~SoftwareRenderBackend() = default;

void SoftwareRenderBackend::executeDisplayList(const DisplayList& list) {
    for (const auto& cmd : list.commands()) {
        switch (cmd.type) {
            case PaintCommandType::FillRect:
                fillRect(cmd.rect, cmd.color);
                break;
            case PaintCommandType::StrokeRect:
                strokeRect(cmd.rect, cmd.color, cmd.number);
                break;
            case PaintCommandType::DrawText:
                drawText(cmd.text, cmd.rect.x, cmd.rect.y, cmd.color, cmd.number);
                break;
            case PaintCommandType::PushClip:
                clipStack_.push_back(cmd.rect);
                break;
            case PaintCommandType::PopClip:
                if (!clipStack_.empty()) clipStack_.pop_back();
                break;
            default:
                break;
        }
    }
}

void SoftwareRenderBackend::fillRect(const Rect& rect, const Color& color) {
    int x0 = std::max(0, static_cast<int>(rect.x));
    int y0 = std::max(0, static_cast<int>(rect.y));
    int x1 = std::min(width_, static_cast<int>(rect.x + rect.width));
    int y1 = std::min(height_, static_cast<int>(rect.y + rect.height));
    
    // Apply clip if any
    if (!clipStack_.empty()) {
        const Rect& clip = clipStack_.back();
        x0 = std::max(x0, static_cast<int>(clip.x));
        y0 = std::max(y0, static_cast<int>(clip.y));
        x1 = std::min(x1, static_cast<int>(clip.x + clip.width));
        y1 = std::min(y1, static_cast<int>(clip.y + clip.height));
    }
    
    uint32_t pixel = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
    
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            pixels_[y * width_ + x] = pixel;
        }
    }
}

void SoftwareRenderBackend::strokeRect(const Rect& rect, const Color& color, float width) {
    float w = width > 0 ? width : 1.0f;
    fillRect({rect.x, rect.y, rect.width, w}, color);
    fillRect({rect.x, rect.y + rect.height - w, rect.width, w}, color);
    fillRect({rect.x, rect.y + w, w, rect.height - 2 * w}, color);
    fillRect({rect.x + rect.width - w, rect.y + w, w, rect.height - 2 * w}, color);
}

void SoftwareRenderBackend::drawText(const std::string& text, float x, float y, 
                                      const Color& color, float fontSize) {
    // Simple placeholder - draw colored rect for text
    float charWidth = fontSize * 0.5f;
    fillRect({x, y, text.length() * charWidth, fontSize}, color);
}

void SoftwareRenderBackend::present() {
    // Handled by platform layer
}

void SoftwareRenderBackend::resize(int width, int height) {
    width_ = width;
    height_ = height;
    pixels_.resize(width * height, 0xFFFFFFFF);
}

void* SoftwareRenderBackend::createTexture(int width, int height) {
    auto* tex = new std::vector<uint32_t>(width * height, 0);
    return tex;
}

void SoftwareRenderBackend::destroyTexture(void* texture) {
    delete static_cast<std::vector<uint32_t>*>(texture);
}

} // namespace Zepra::WebCore
