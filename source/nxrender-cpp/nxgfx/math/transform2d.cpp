// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "transform2d.h"
#include <cmath>

namespace NXRender {
namespace Math {

Transform2D::Transform2D() : matrix(Matrix3x3::identity()) {}
Transform2D::Transform2D(const Matrix3x3& m) : matrix(m) {}

Transform2D Transform2D::identity() { return Transform2D(); }

Transform2D Transform2D::translation(float x, float y) {
    Transform2D t;
    t.matrix.set(0, 2, x);
    t.matrix.set(1, 2, y);
    return t;
}

Transform2D Transform2D::rotation(float angle) {
    Transform2D t;
    float c = std::cos(angle), s = std::sin(angle);
    t.matrix.set(0, 0, c); t.matrix.set(0, 1, -s);
    t.matrix.set(1, 0, s); t.matrix.set(1, 1, c);
    return t;
}

Transform2D Transform2D::scale(float sx, float sy) {
    Transform2D t;
    t.matrix.set(0, 0, sx);
    t.matrix.set(1, 1, sy);
    return t;
}

Transform2D Transform2D::operator*(const Transform2D& other) const {
    return Transform2D(matrix * other.matrix);
}

Vector2 Transform2D::mapPoint(const Vector2& point) const {
    return matrix * point;
}

Transform2D Transform2D::inverse() const {
    return Transform2D(matrix.inverse());
}

void Transform2D::translate(float x, float y) {
    *this = translation(x, y) * (*this);
}

void Transform2D::rotate(float angle) {
    *this = rotation(angle) * (*this);
}

void Transform2D::scaleSelf(float sx, float sy) {
    *this = scale(sx, sy) * (*this);
}

} // namespace Math
} // namespace NXRender
