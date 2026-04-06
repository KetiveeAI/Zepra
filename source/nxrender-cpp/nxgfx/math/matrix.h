// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "vector.h"
#include <array>
#include <cstring>

namespace NXRender {
namespace Math {

struct Matrix3x3 {
    std::array<float, 9> m;

    Matrix3x3();
    
    static Matrix3x3 identity();
    
    float get(int row, int col) const { return m[col * 3 + row]; }
    void set(int row, int col, float v) { m[col * 3 + row] = v; }
    
    Matrix3x3 operator*(const Matrix3x3& other) const;
    Vector2 operator*(const Vector2& v) const;
    
    Matrix3x3 inverse() const;
    float determinant() const;
};

struct Matrix4x4 {
    std::array<float, 16> m;

    Matrix4x4();
    static Matrix4x4 identity();
    
    float get(int row, int col) const { return m[col * 4 + row]; }
    void set(int row, int col, float v) { m[col * 4 + row] = v; }
    
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Vector4 operator*(const Vector4& v) const;
    Vector3 multiplyPoint(const Vector3& v) const;
    Vector3 multiplyDirection(const Vector3& v) const;
    
    Matrix4x4 inverse() const;
    float determinant() const;
    
    // Projections
    static Matrix4x4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    static Matrix4x4 perspective(float fovY, float aspect, float zNear, float zFar);
    static Matrix4x4 lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
};

} // namespace Math
} // namespace NXRender
