// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "transform3d.h"

namespace NXRender {
namespace Math {

Transform3D::Transform3D() : matrix(Matrix4x4::identity()) {}
Transform3D::Transform3D(const Matrix4x4& m) : matrix(m) {}

Transform3D Transform3D::identity() { return Transform3D(); }

Transform3D Transform3D::translation(const Vector3& v) {
    Transform3D t;
    t.matrix.set(0, 3, v.x);
    t.matrix.set(1, 3, v.y);
    t.matrix.set(2, 3, v.z);
    return t;
}

Transform3D Transform3D::rotation(const Quaternion& q) {
    return Transform3D(q.toMatrix());
}

Transform3D Transform3D::scale(const Vector3& v) {
    Transform3D t;
    t.matrix.set(0, 0, v.x);
    t.matrix.set(1, 1, v.y);
    t.matrix.set(2, 2, v.z);
    return t;
}

Transform3D Transform3D::operator*(const Transform3D& other) const {
    return Transform3D(matrix * other.matrix);
}

Vector3 Transform3D::mapPoint(const Vector3& point) const {
    return matrix.multiplyPoint(point);
}

Transform3D Transform3D::inverse() const {
    return Transform3D(matrix.inverse());
}

void Transform3D::translate(const Vector3& v) {
    *this = translation(v) * (*this);
}

void Transform3D::rotate(const Quaternion& q) {
    *this = rotation(q) * (*this);
}

void Transform3D::scaleSelf(const Vector3& v) {
    *this = scale(v) * (*this);
}

} // namespace Math
} // namespace NXRender
