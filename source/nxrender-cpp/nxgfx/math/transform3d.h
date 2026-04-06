// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "matrix.h"
#include "quaternion.h"

namespace NXRender {
namespace Math {

struct Transform3D {
    Matrix4x4 matrix;

    Transform3D();
    explicit Transform3D(const Matrix4x4& m);

    static Transform3D identity();
    static Transform3D translation(const Vector3& v);
    static Transform3D rotation(const Quaternion& q);
    static Transform3D scale(const Vector3& v);

    Transform3D operator*(const Transform3D& other) const;
    Vector3 mapPoint(const Vector3& point) const;

    Transform3D inverse() const;
    void translate(const Vector3& v);
    void rotate(const Quaternion& q);
    void scaleSelf(const Vector3& v);
};

} // namespace Math
} // namespace NXRender
