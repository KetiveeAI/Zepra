// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "vector.h"
#include "matrix.h"

namespace NXRender {
namespace Math {

struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    Quaternion() = default;
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quaternion identity();
    static Quaternion fromAxisAngle(const Vector3& axis, float angle);
    static Quaternion fromEuler(float roll, float pitch, float yaw);

    Quaternion operator*(const Quaternion& q) const;
    Vector3 operator*(const Vector3& v) const;

    Quaternion normalized() const;
    Quaternion conjugate() const;
    Quaternion inverse() const;

    Matrix4x4 toMatrix() const;
};

} // namespace Math
} // namespace NXRender
