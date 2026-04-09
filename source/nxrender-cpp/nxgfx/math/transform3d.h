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

    // Factory constructors
    static Transform3D identity();
    static Transform3D translation(const Vector3& v);
    static Transform3D translation(float x, float y, float z);
    static Transform3D rotation(const Quaternion& q);
    static Transform3D rotationX(float angleRadians);
    static Transform3D rotationY(float angleRadians);
    static Transform3D rotationZ(float angleRadians);
    static Transform3D rotationAxisAngle(const Vector3& axis, float angleRadians);
    static Transform3D scale(const Vector3& v);
    static Transform3D scale(float sx, float sy, float sz);
    static Transform3D scale(float uniform);

    // Projection matrices
    static Transform3D perspective(float fovYRadians, float aspect, float near, float far);
    static Transform3D perspectiveCSS(float distance);
    static Transform3D orthographic(float left, float right, float bottom, float top,
                                     float near, float far);
    static Transform3D lookAt(const Vector3& eye, const Vector3& target, const Vector3& up);

    // Interpolation
    static Transform3D interpolate(const Transform3D& from, const Transform3D& to, float t);

    // Operators
    Transform3D operator*(const Transform3D& other) const;
    bool operator==(const Transform3D& other) const;

    // Point mapping
    Vector3 mapPoint(const Vector3& point) const;
    Vector3 mapDirection(const Vector3& dir) const;

    // Inverse
    Transform3D inverse() const;
    Transform3D transpose() const;

    // Queries
    bool isIdentity() const;
    bool isAffine() const;
    bool is2D() const;
    float determinant() const;

    // Decomposition
    bool decompose(Vector3& outTranslation, Quaternion& outRotation,
                   Vector3& outScale) const;

    // In-place mutation
    void translate(const Vector3& v);
    void rotate(const Quaternion& q);
    void scaleSelf(const Vector3& v);
    void preTranslate(const Vector3& v);
    void preRotate(const Quaternion& q);
    void preScale(const Vector3& v);
};

} // namespace Math
} // namespace NXRender
