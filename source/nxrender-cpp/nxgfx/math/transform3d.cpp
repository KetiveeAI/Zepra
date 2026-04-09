// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "transform3d.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

Transform3D Transform3D::translation(float x, float y, float z) {
    return translation(Vector3(x, y, z));
}

Transform3D Transform3D::rotation(const Quaternion& q) {
    return Transform3D(q.toMatrix());
}

Transform3D Transform3D::rotationX(float angle) {
    Transform3D t;
    float c = std::cos(angle), s = std::sin(angle);
    t.matrix.set(1, 1, c);  t.matrix.set(1, 2, -s);
    t.matrix.set(2, 1, s);  t.matrix.set(2, 2, c);
    return t;
}

Transform3D Transform3D::rotationY(float angle) {
    Transform3D t;
    float c = std::cos(angle), s = std::sin(angle);
    t.matrix.set(0, 0, c);  t.matrix.set(0, 2, s);
    t.matrix.set(2, 0, -s); t.matrix.set(2, 2, c);
    return t;
}

Transform3D Transform3D::rotationZ(float angle) {
    Transform3D t;
    float c = std::cos(angle), s = std::sin(angle);
    t.matrix.set(0, 0, c);  t.matrix.set(0, 1, -s);
    t.matrix.set(1, 0, s);  t.matrix.set(1, 1, c);
    return t;
}

Transform3D Transform3D::rotationAxisAngle(const Vector3& axis, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    float t = 1.0f - c;

    // Normalize axis
    float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 1e-6f) return identity();
    float x = axis.x / len, y = axis.y / len, z = axis.z / len;

    Transform3D result;
    result.matrix.set(0, 0, t * x * x + c);
    result.matrix.set(0, 1, t * x * y - s * z);
    result.matrix.set(0, 2, t * x * z + s * y);

    result.matrix.set(1, 0, t * x * y + s * z);
    result.matrix.set(1, 1, t * y * y + c);
    result.matrix.set(1, 2, t * y * z - s * x);

    result.matrix.set(2, 0, t * x * z - s * y);
    result.matrix.set(2, 1, t * y * z + s * x);
    result.matrix.set(2, 2, t * z * z + c);

    return result;
}

Transform3D Transform3D::scale(const Vector3& v) {
    Transform3D t;
    t.matrix.set(0, 0, v.x);
    t.matrix.set(1, 1, v.y);
    t.matrix.set(2, 2, v.z);
    return t;
}

Transform3D Transform3D::scale(float sx, float sy, float sz) {
    return scale(Vector3(sx, sy, sz));
}

Transform3D Transform3D::scale(float uniform) {
    return scale(Vector3(uniform, uniform, uniform));
}

Transform3D Transform3D::perspective(float fovY, float aspect, float near, float far) {
    float tanHalfFov = std::tan(fovY * 0.5f);
    if (std::abs(tanHalfFov) < 1e-6f || std::abs(aspect) < 1e-6f) return identity();

    float f = 1.0f / tanHalfFov;
    float nf = near - far;
    if (std::abs(nf) < 1e-6f) return identity();

    Transform3D t;
    // Zero out identity first
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            t.matrix.set(r, c, 0);

    t.matrix.set(0, 0, f / aspect);
    t.matrix.set(1, 1, f);
    t.matrix.set(2, 2, (far + near) / nf);
    t.matrix.set(2, 3, (2.0f * far * near) / nf);
    t.matrix.set(3, 2, -1.0f);

    return t;
}

Transform3D Transform3D::perspectiveCSS(float distance) {
    // CSS perspective: matrix3d with perspective projection
    // perspective(d) => matrix where m[3][2] = -1/d
    if (std::abs(distance) < 1e-6f) return identity();

    Transform3D t;
    t.matrix.set(3, 2, -1.0f / distance);
    return t;
}

Transform3D Transform3D::orthographic(float left, float right, float bottom, float top,
                                       float near, float far) {
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    if (std::abs(rl) < 1e-6f || std::abs(tb) < 1e-6f || std::abs(fn) < 1e-6f) return identity();

    Transform3D t;
    // Zero out
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            t.matrix.set(r, c, 0);

    t.matrix.set(0, 0, 2.0f / rl);
    t.matrix.set(1, 1, 2.0f / tb);
    t.matrix.set(2, 2, -2.0f / fn);
    t.matrix.set(0, 3, -(right + left) / rl);
    t.matrix.set(1, 3, -(top + bottom) / tb);
    t.matrix.set(2, 3, -(far + near) / fn);
    t.matrix.set(3, 3, 1.0f);

    return t;
}

Transform3D Transform3D::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 forward = (target - eye).normalized();
    Vector3 right = forward.cross(up).normalized();
    Vector3 newUp = right.cross(forward);

    Transform3D t;
    t.matrix.set(0, 0, right.x);    t.matrix.set(0, 1, right.y);    t.matrix.set(0, 2, right.z);
    t.matrix.set(1, 0, newUp.x);    t.matrix.set(1, 1, newUp.y);    t.matrix.set(1, 2, newUp.z);
    t.matrix.set(2, 0, -forward.x); t.matrix.set(2, 1, -forward.y); t.matrix.set(2, 2, -forward.z);

    t.matrix.set(0, 3, -right.dot(eye));
    t.matrix.set(1, 3, -newUp.dot(eye));
    t.matrix.set(2, 3, forward.dot(eye));

    return t;
}

Transform3D Transform3D::interpolate(const Transform3D& from, const Transform3D& to, float t) {
    Vector3 fromTrans, toTrans, fromScale, toScale;
    Quaternion fromRot, toRot;

    if (!from.decompose(fromTrans, fromRot, fromScale) ||
        !to.decompose(toTrans, toRot, toScale)) {
        // Fallback: per-element lerp
        Transform3D result;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                float val = from.matrix.get(r, c) * (1.0f - t) + to.matrix.get(r, c) * t;
                result.matrix.set(r, c, val);
            }
        }
        return result;
    }

    Vector3 trans(fromTrans.x + (toTrans.x - fromTrans.x) * t,
                  fromTrans.y + (toTrans.y - fromTrans.y) * t,
                  fromTrans.z + (toTrans.z - fromTrans.z) * t);

    Vector3 sc(fromScale.x + (toScale.x - fromScale.x) * t,
               fromScale.y + (toScale.y - fromScale.y) * t,
               fromScale.z + (toScale.z - fromScale.z) * t);

    Quaternion rot = Quaternion::slerp(fromRot, toRot, t);

    return translation(trans) * rotation(rot) * scale(sc);
}

Transform3D Transform3D::operator*(const Transform3D& other) const {
    return Transform3D(matrix * other.matrix);
}

bool Transform3D::operator==(const Transform3D& other) const {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (std::abs(matrix.get(r, c) - other.matrix.get(r, c)) > 1e-6f) return false;
        }
    }
    return true;
}

Vector3 Transform3D::mapPoint(const Vector3& point) const {
    return matrix.multiplyPoint(point);
}

Vector3 Transform3D::mapDirection(const Vector3& dir) const {
    // Direction: no translation, no perspective divide
    float x = matrix.get(0, 0) * dir.x + matrix.get(0, 1) * dir.y + matrix.get(0, 2) * dir.z;
    float y = matrix.get(1, 0) * dir.x + matrix.get(1, 1) * dir.y + matrix.get(1, 2) * dir.z;
    float z = matrix.get(2, 0) * dir.x + matrix.get(2, 1) * dir.y + matrix.get(2, 2) * dir.z;
    return Vector3(x, y, z);
}

Transform3D Transform3D::inverse() const {
    return Transform3D(matrix.inverse());
}

Transform3D Transform3D::transpose() const {
    Transform3D t;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            t.matrix.set(r, c, matrix.get(c, r));
        }
    }
    return t;
}

bool Transform3D::isIdentity() const {
    return *this == identity();
}

bool Transform3D::isAffine() const {
    return std::abs(matrix.get(3, 0)) < 1e-6f &&
           std::abs(matrix.get(3, 1)) < 1e-6f &&
           std::abs(matrix.get(3, 2)) < 1e-6f &&
           std::abs(matrix.get(3, 3) - 1.0f) < 1e-6f;
}

bool Transform3D::is2D() const {
    return std::abs(matrix.get(2, 0)) < 1e-6f &&
           std::abs(matrix.get(2, 1)) < 1e-6f &&
           std::abs(matrix.get(0, 2)) < 1e-6f &&
           std::abs(matrix.get(1, 2)) < 1e-6f &&
           std::abs(matrix.get(2, 2) - 1.0f) < 1e-6f &&
           std::abs(matrix.get(2, 3)) < 1e-6f &&
           std::abs(matrix.get(3, 2)) < 1e-6f;
}

float Transform3D::determinant() const {
    return matrix.determinant();
}

bool Transform3D::decompose(Vector3& outTranslation, Quaternion& outRotation,
                              Vector3& outScale) const {
    // Extract translation
    outTranslation.x = matrix.get(0, 3);
    outTranslation.y = matrix.get(1, 3);
    outTranslation.z = matrix.get(2, 3);

    // Extract scale from column lengths
    Vector3 col0(matrix.get(0, 0), matrix.get(1, 0), matrix.get(2, 0));
    Vector3 col1(matrix.get(0, 1), matrix.get(1, 1), matrix.get(2, 1));
    Vector3 col2(matrix.get(0, 2), matrix.get(1, 2), matrix.get(2, 2));

    float sx = col0.length();
    float sy = col1.length();
    float sz = col2.length();

    if (sx < 1e-6f || sy < 1e-6f || sz < 1e-6f) return false;

    // Check for reflection
    Vector3 cross = col0.cross(col1);
    if (cross.dot(col2) < 0) sx = -sx;

    outScale = Vector3(sx, sy, sz);

    // Normalize columns to get rotation matrix
    Matrix4x4 rotMat = Matrix4x4::identity();
    rotMat.set(0, 0, col0.x / sx); rotMat.set(0, 1, col1.x / sy); rotMat.set(0, 2, col2.x / sz);
    rotMat.set(1, 0, col0.y / sx); rotMat.set(1, 1, col1.y / sy); rotMat.set(1, 2, col2.y / sz);
    rotMat.set(2, 0, col0.z / sx); rotMat.set(2, 1, col1.z / sy); rotMat.set(2, 2, col2.z / sz);

    outRotation = Quaternion::fromMatrix(rotMat);
    return true;
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

void Transform3D::preTranslate(const Vector3& v) {
    *this = (*this) * translation(v);
}

void Transform3D::preRotate(const Quaternion& q) {
    *this = (*this) * rotation(q);
}

void Transform3D::preScale(const Vector3& v) {
    *this = (*this) * scale(v);
}

} // namespace Math
} // namespace NXRender
