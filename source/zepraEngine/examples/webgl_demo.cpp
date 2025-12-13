/**
 * @file webgl_demo.cpp
 * @brief WebGL 3D Rotating Cube Demo
 * 
 * Demonstrates the WebGLRenderingContext API with:
 * - Shader compilation and linking
 * - Vertex buffer creation
 * - Matrix uniform manipulation
 * - 3D cube rendering with rotation
 */

#include "webcore/webgl_context.hpp"
#include "webcore/sdl_gl_context.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <chrono>
#include <array>

using namespace Zepra::WebCore;

// ===========================================================================
// Shaders
// ===========================================================================

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

out vec3 vColor;

void main() {
    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ===========================================================================
// Matrix Math Helpers
// ===========================================================================

void mat4Identity(float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void mat4Perspective(float* m, float fov, float aspect, float near, float far) {
    float tanHalfFov = std::tan(fov / 2.0f);
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = 1.0f / (aspect * tanHalfFov);
    m[5] = 1.0f / tanHalfFov;
    m[10] = -(far + near) / (far - near);
    m[11] = -1.0f;
    m[14] = -(2.0f * far * near) / (far - near);
}

void mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ, 
                float centerX, float centerY, float centerZ,
                float upX, float upY, float upZ) {
    float fx = centerX - eyeX, fy = centerY - eyeY, fz = centerZ - eyeZ;
    float fLen = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx /= fLen; fy /= fLen; fz /= fLen;
    
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;
    float sLen = std::sqrt(sx*sx + sy*sy + sz*sz);
    sx /= sLen; sy /= sLen; sz /= sLen;
    
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;
    
    m[0] = sx; m[1] = ux; m[2] = -fx; m[3] = 0;
    m[4] = sy; m[5] = uy; m[6] = -fy; m[7] = 0;
    m[8] = sz; m[9] = uz; m[10] = -fz; m[11] = 0;
    m[12] = -(sx*eyeX + sy*eyeY + sz*eyeZ);
    m[13] = -(ux*eyeX + uy*eyeY + uz*eyeZ);
    m[14] = (fx*eyeX + fy*eyeY + fz*eyeZ);
    m[15] = 1;
}

void mat4RotateY(float* m, float angle) {
    mat4Identity(m);
    m[0] = std::cos(angle);
    m[2] = std::sin(angle);
    m[8] = -std::sin(angle);
    m[10] = std::cos(angle);
}

void mat4RotateX(float* m, float angle) {
    mat4Identity(m);
    m[5] = std::cos(angle);
    m[6] = -std::sin(angle);
    m[9] = std::sin(angle);
    m[10] = std::cos(angle);
}

void mat4Multiply(float* result, const float* a, const float* b) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

// ===========================================================================
// Cube Geometry
// ===========================================================================

// Cube vertices: position (3) + color (3) = 6 floats per vertex
// 36 vertices for 6 faces * 2 triangles * 3 vertices
const float cubeVertices[] = {
    // Front face (red)
    -0.5f, -0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.3f, 0.3f,
    
    // Back face (green)
    -0.5f, -0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
    -0.5f,  0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
     0.5f,  0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
    -0.5f, -0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
     0.5f,  0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
     0.5f, -0.5f, -0.5f,  0.3f, 1.0f, 0.3f,
    
    // Top face (blue)
    -0.5f,  0.5f, -0.5f,  0.3f, 0.3f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.3f, 0.3f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.3f, 0.3f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.3f, 0.3f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.3f, 0.3f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.3f, 0.3f, 1.0f,
    
    // Bottom face (yellow)
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.3f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.3f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.3f,
    -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.3f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.3f,
    -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.3f,
    
    // Right face (cyan)
     0.5f, -0.5f, -0.5f,  0.3f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.3f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.3f, 1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.3f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.3f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.3f, 1.0f, 1.0f,
    
    // Left face (magenta)
    -0.5f, -0.5f, -0.5f,  1.0f, 0.3f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.3f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.3f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.3f, 1.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.3f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.3f, 1.0f,
};

// ===========================================================================
// Main Demo
// ===========================================================================

class WebGLDemo {
public:
    bool init() {
        // Initialize SDL with OpenGL context
        if (!context_.initialize("ZepraBrowser - WebGL 3D Demo", 1024, 768, RenderMode::OpenGL)) {
            std::cerr << "Failed to initialize SDL GL context" << std::endl;
            return false;
        }
        
        // Initialize WebGL context
        if (!gl_.initialize(1024, 768)) {
            std::cerr << "Failed to initialize WebGL context" << std::endl;
            return false;
        }
        
        std::cout << "WebGL context initialized successfully" << std::endl;
        
        // Create shaders
        if (!createShaderProgram()) {
            return false;
        }
        
        // Create vertex buffer
        createVertexBuffer();
        
        return true;
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        auto startTime = std::chrono::steady_clock::now();
        
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
            }
            
            auto now = std::chrono::steady_clock::now();
            float time = std::chrono::duration<float>(now - startTime).count();
            
            render(time);
            
            SDL_Delay(16);
        }
    }
    
private:
    bool createShaderProgram() {
        // Create vertex shader
        auto vs = gl_.createShader(WebGLConstants::VERTEX_SHADER);
        gl_.shaderSource(vs, vertexShaderSource);
        gl_.compileShader(vs);
        
        if (!gl_.getShaderParameter(vs, WebGLConstants::COMPILE_STATUS)) {
            std::cerr << "Vertex shader error: " << gl_.getShaderInfoLog(vs) << std::endl;
            return false;
        }
        
        // Create fragment shader
        auto fs = gl_.createShader(WebGLConstants::FRAGMENT_SHADER);
        gl_.shaderSource(fs, fragmentShaderSource);
        gl_.compileShader(fs);
        
        if (!gl_.getShaderParameter(fs, WebGLConstants::COMPILE_STATUS)) {
            std::cerr << "Fragment shader error: " << gl_.getShaderInfoLog(fs) << std::endl;
            return false;
        }
        
        // Create program
        program_ = gl_.createProgram();
        gl_.attachShader(program_, vs);
        gl_.attachShader(program_, fs);
        gl_.linkProgram(program_);
        
        if (!gl_.getProgramParameter(program_, WebGLConstants::LINK_STATUS)) {
            std::cerr << "Program link error" << std::endl;
            return false;
        }
        
        // Get uniform locations
        modelMatrixLoc_ = gl_.getUniformLocation(program_, "uModelMatrix");
        viewMatrixLoc_ = gl_.getUniformLocation(program_, "uViewMatrix");
        projMatrixLoc_ = gl_.getUniformLocation(program_, "uProjectionMatrix");
        
        std::cout << "Shader program created successfully" << std::endl;
        return true;
    }
    
    void createVertexBuffer() {
        vbo_ = gl_.createBuffer();
        gl_.bindBuffer(WebGLConstants::ARRAY_BUFFER, vbo_);
        gl_.bufferData(WebGLConstants::ARRAY_BUFFER, cubeVertices, sizeof(cubeVertices), 
                       WebGLConstants::STATIC_DRAW);
        
        std::cout << "Vertex buffer created" << std::endl;
    }
    
    void render(float time) {
        // Clear
        gl_.clearColor(0.1f, 0.1f, 0.15f, 1.0f);
        gl_.clear(WebGLConstants::COLOR_BUFFER_BIT | WebGLConstants::DEPTH_BUFFER_BIT);
        gl_.enable(WebGLConstants::DEPTH_TEST);
        
        // Use program
        gl_.useProgram(program_);
        
        // Setup matrices
        float projection[16], view[16], model[16], rotX[16], rotY[16], temp[16];
        
        mat4Perspective(projection, 45.0f * 3.14159f / 180.0f, 1024.0f / 768.0f, 0.1f, 100.0f);
        mat4LookAt(view, 0, 0, 3, 0, 0, 0, 0, 1, 0);
        mat4RotateY(rotY, time);
        mat4RotateX(rotX, time * 0.7f);
        mat4Multiply(model, rotY, rotX);
        
        gl_.uniformMatrix4fv(projMatrixLoc_, 0, projection);
        gl_.uniformMatrix4fv(viewMatrixLoc_, 0, view);
        gl_.uniformMatrix4fv(modelMatrixLoc_, 0, model);
        
        // Bind VBO and set attributes
        gl_.bindBuffer(WebGLConstants::ARRAY_BUFFER, vbo_);
        gl_.vertexAttribPointer(0, 3, WebGLConstants::FLOAT, 0, 6 * sizeof(float), 0);
        gl_.enableVertexAttribArray(0);
        gl_.vertexAttribPointer(1, 3, WebGLConstants::FLOAT, 0, 6 * sizeof(float), 3 * sizeof(float));
        gl_.enableVertexAttribArray(1);
        
        // Draw cube
        gl_.drawArrays(WebGLConstants::TRIANGLES, 0, 36);
        
        // Swap buffers
        context_.endFrame();
    }
    
    SDLGLContext context_;
    WebGLRenderingContext gl_;
    WebGLProgram program_;
    WebGLBuffer vbo_;
    WebGLUniformLocation modelMatrixLoc_;
    WebGLUniformLocation viewMatrixLoc_;
    WebGLUniformLocation projMatrixLoc_;
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    std::cout << "=== ZepraBrowser WebGL 3D Demo ===" << std::endl;
    std::cout << "Press ESC to exit." << std::endl;
    std::cout << std::endl;
    
    WebGLDemo demo;
    if (demo.init()) {
        demo.run();
        std::cout << "Demo finished successfully!" << std::endl;
    } else {
        std::cerr << "Failed to initialize demo" << std::endl;
        return 1;
    }
    
    SDL_Quit();
    return 0;
}
