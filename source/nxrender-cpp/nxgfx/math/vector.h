// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <cmath>

namespace NXRender {
namespace Math {

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x_, float y_) : x(x_), y(y_) {}

    Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }

    Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
    Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
    Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

    float lengthSquared() const { return x*x + y*y; }
    float length() const { return std::sqrt(lengthSquared()); }
    
    Vector2 normalized() const {
        float len = length();
        if (len > 0.0f) return *this / len;
        return Vector2(0, 0);
    }
    
    float dot(const Vector2& v) const { return x * v.x + y * v.y; }
    float cross(const Vector2& v) const { return x * v.y - y * v.x; }
};

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

    float lengthSquared() const { return x*x + y*y + z*z; }
    float length() const { return std::sqrt(lengthSquared()); }
    
    Vector3 normalized() const {
        float len = length();
        if (len > 0.0f) return *this / len;
        return Vector3(0, 0, 0);
    }
    
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }
};

struct Vector4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    Vector4() = default;
    Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    
    Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4 operator*(float s) const { return Vector4(x * s, y * s, z * s, w * s); }
    
    float dot(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
};

} // namespace Math
} // namespace NXRender
