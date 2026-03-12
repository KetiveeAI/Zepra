// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file context3d.h
 * @brief 3D Graphics context for NXRender
 */

#pragma once

#include "color.h"
#include "primitives.h"
#include <cmath>
#include <vector>
#include <memory>

namespace NXRender {

/**
 * @brief 3D Vector
 */
struct Vec3 {
    float x = 0, y = 0, z = 0;
    
    constexpr Vec3() = default;
    constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
    
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const { float l = length(); return l > 0 ? *this / l : Vec3(); }
    
    static Vec3 zero() { return Vec3(0, 0, 0); }
    static Vec3 up() { return Vec3(0, 1, 0); }
    static Vec3 forward() { return Vec3(0, 0, -1); }
    static Vec3 right() { return Vec3(1, 0, 0); }
};

/**
 * @brief 4x4 Matrix for transforms
 */
struct Mat4 {
    float m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    
    Mat4() = default;
    
    static Mat4 identity() { return Mat4(); }
    
    static Mat4 translate(float x, float y, float z) {
        Mat4 m;
        m.m[12] = x; m.m[13] = y; m.m[14] = z;
        return m;
    }
    
    static Mat4 scale(float x, float y, float z) {
        Mat4 m;
        m.m[0] = x; m.m[5] = y; m.m[10] = z;
        return m;
    }
    
    static Mat4 rotateX(float rad) {
        Mat4 m;
        float c = std::cos(rad), s = std::sin(rad);
        m.m[5] = c; m.m[6] = s; m.m[9] = -s; m.m[10] = c;
        return m;
    }
    
    static Mat4 rotateY(float rad) {
        Mat4 m;
        float c = std::cos(rad), s = std::sin(rad);
        m.m[0] = c; m.m[2] = -s; m.m[8] = s; m.m[10] = c;
        return m;
    }
    
    static Mat4 rotateZ(float rad) {
        Mat4 m;
        float c = std::cos(rad), s = std::sin(rad);
        m.m[0] = c; m.m[1] = s; m.m[4] = -s; m.m[5] = c;
        return m;
    }
    
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 m;
        float tanHalf = std::tan(fov / 2);
        m.m[0] = 1.0f / (aspect * tanHalf);
        m.m[5] = 1.0f / tanHalf;
        m.m[10] = -(far + near) / (far - near);
        m.m[11] = -1.0f;
        m.m[14] = -(2.0f * far * near) / (far - near);
        m.m[15] = 0;
        return m;
    }
    
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 r = f.cross(up).normalized();
        Vec3 u = r.cross(f);
        Mat4 m;
        m.m[0] = r.x; m.m[4] = r.y; m.m[8] = r.z;
        m.m[1] = u.x; m.m[5] = u.y; m.m[9] = u.z;
        m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
        m.m[12] = -r.dot(eye); m.m[13] = -u.dot(eye); m.m[14] = f.dot(eye);
        return m;
    }
    
    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                r.m[i + j * 4] = 0;
                for (int k = 0; k < 4; k++) {
                    r.m[i + j * 4] += m[i + k * 4] * b.m[k + j * 4];
                }
            }
        }
        return r;
    }
    
    Vec3 transform(const Vec3& v) const {
        float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
        return Vec3(
            (m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12]) / w,
            (m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13]) / w,
            (m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]) / w
        );
    }
};

/**
 * @brief Vertex with position, normal, color, UV
 */
struct Vertex3D {
    Vec3 position;
    Vec3 normal;
    Color color = Color::white();
    float u = 0, v = 0;  // Texture coordinates
};

/**
 * @brief Simple mesh (triangle list)
 */
struct Mesh {
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    
    void clear() { vertices.clear(); indices.clear(); }
    
    // Generate primitives
    static Mesh cube(float size = 1.0f);
    static Mesh sphere(float radius = 0.5f, int segments = 16);
    static Mesh plane(float width = 1.0f, float height = 1.0f);
    static Mesh cylinder(float radius = 0.5f, float height = 1.0f, int segments = 16);
};

/**
 * @brief Camera for 3D scenes
 */
class Camera {
public:
    Vec3 position = Vec3(0, 0, 5);
    Vec3 target = Vec3::zero();
    Vec3 up = Vec3::up();
    
    float fov = 60.0f * 3.14159f / 180.0f;  // In radians
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
    
    Mat4 viewMatrix() const { return Mat4::lookAt(position, target, up); }
    Mat4 projectionMatrix() const { return Mat4::perspective(fov, aspectRatio, nearPlane, farPlane); }
    Mat4 viewProjection() const { return projectionMatrix() * viewMatrix(); }
    
    // Orbit around target
    void orbit(float yaw, float pitch) {
        Vec3 dir = position - target;
        float dist = dir.length();
        
        // Convert to spherical
        float theta = std::atan2(dir.x, dir.z) + yaw;
        float phi = std::acos(dir.y / dist);
        phi = std::max(0.1f, std::min(3.04f, phi + pitch));  // Clamp
        
        // Back to cartesian
        position.x = target.x + dist * std::sin(phi) * std::sin(theta);
        position.y = target.y + dist * std::cos(phi);
        position.z = target.z + dist * std::sin(phi) * std::cos(theta);
    }
    
    void zoom(float delta) {
        Vec3 dir = (position - target).normalized();
        float dist = (position - target).length();
        dist = std::max(0.5f, dist - delta);
        position = target + dir * dist;
    }
};

/**
 * @brief 3D Graphics context (extends GpuContext)
 */
class GpuContext3D {
public:
    GpuContext3D();
    ~GpuContext3D();
    
    void begin3D(const Camera& camera);
    void end3D();
    
    // Lighting
    void setAmbientLight(const Color& color);
    void setDirectionalLight(const Vec3& direction, const Color& color);
    
    // Drawing 3D primitives
    void drawMesh(const Mesh& mesh, const Mat4& transform = Mat4::identity());
    void drawCube(const Vec3& center, float size, const Color& color);
    void drawSphere(const Vec3& center, float radius, const Color& color);
    void drawLine3D(const Vec3& start, const Vec3& end, const Color& color);
    void drawGrid(float size = 10.0f, int divisions = 10);
    
    // Wireframe mode
    void setWireframe(bool enabled) { wireframe_ = enabled; }
    bool wireframe() const { return wireframe_; }
    
private:
    Camera camera_;
    bool wireframe_ = false;
    Color ambientLight_ = Color(50, 50, 50);
    Vec3 lightDir_ = Vec3(0.5f, 1.0f, 0.3f).normalized();
    Color lightColor_ = Color::white();
};

} // namespace NXRender
