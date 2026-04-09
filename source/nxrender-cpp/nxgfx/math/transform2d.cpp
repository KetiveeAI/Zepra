// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "transform2d.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

Transform2D Transform2D::skewX(float angle) {
    Transform2D t;
    t.matrix.set(0, 1, std::tan(angle));
    return t;
}

Transform2D Transform2D::skewY(float angle) {
    Transform2D t;
    t.matrix.set(1, 0, std::tan(angle));
    return t;
}

Transform2D Transform2D::skew(float angleX, float angleY) {
    Transform2D t;
    t.matrix.set(0, 1, std::tan(angleX));
    t.matrix.set(1, 0, std::tan(angleY));
    return t;
}

Transform2D Transform2D::fromCSS(float a, float b, float c, float d, float e, float f) {
    // CSS matrix(a, b, c, d, e, f) maps to:
    // | a c e |
    // | b d f |
    // | 0 0 1 |
    Transform2D t;
    t.matrix.set(0, 0, a); t.matrix.set(0, 1, c); t.matrix.set(0, 2, e);
    t.matrix.set(1, 0, b); t.matrix.set(1, 1, d); t.matrix.set(1, 2, f);
    t.matrix.set(2, 0, 0); t.matrix.set(2, 1, 0); t.matrix.set(2, 2, 1);
    return t;
}

Transform2D Transform2D::rotationAround(float angle, float cx, float cy) {
    return translation(cx, cy) * rotation(angle) * translation(-cx, -cy);
}

Transform2D Transform2D::scaleAround(float sx, float sy, float cx, float cy) {
    return translation(cx, cy) * scale(sx, sy) * translation(-cx, -cy);
}

Transform2D Transform2D::operator*(const Transform2D& other) const {
    return Transform2D(matrix * other.matrix);
}

bool Transform2D::operator==(const Transform2D& other) const {
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (std::abs(matrix.get(r, c) - other.matrix.get(r, c)) > 1e-6f) return false;
        }
    }
    return true;
}

Vector2 Transform2D::mapPoint(const Vector2& point) const {
    return matrix * point;
}

void Transform2D::mapPoints(const Vector2* src, Vector2* dst, size_t count) const {
    for (size_t i = 0; i < count; i++) {
        dst[i] = mapPoint(src[i]);
    }
}

Rect Transform2D::mapRect(const Rect& rect) const {
    Vector2 corners[4] = {
        Vector2(rect.x, rect.y),
        Vector2(rect.x + rect.width, rect.y),
        Vector2(rect.x + rect.width, rect.y + rect.height),
        Vector2(rect.x, rect.y + rect.height)
    };

    Vector2 mapped[4];
    mapPoints(corners, mapped, 4);

    float minX = mapped[0].x, maxX = mapped[0].x;
    float minY = mapped[0].y, maxY = mapped[0].y;
    for (int i = 1; i < 4; i++) {
        minX = std::min(minX, mapped[i].x);
        maxX = std::max(maxX, mapped[i].x);
        minY = std::min(minY, mapped[i].y);
        maxY = std::max(maxY, mapped[i].y);
    }

    return Rect(minX, minY, maxX - minX, maxY - minY);
}

Transform2D Transform2D::inverse() const {
    return Transform2D(matrix.inverse());
}

bool Transform2D::isIdentity() const {
    return *this == identity();
}

bool Transform2D::isTranslation() const {
    return std::abs(matrix.get(0, 0) - 1.0f) < 1e-6f &&
           std::abs(matrix.get(1, 1) - 1.0f) < 1e-6f &&
           std::abs(matrix.get(0, 1)) < 1e-6f &&
           std::abs(matrix.get(1, 0)) < 1e-6f;
}

bool Transform2D::isScaleTranslation() const {
    return std::abs(matrix.get(0, 1)) < 1e-6f &&
           std::abs(matrix.get(1, 0)) < 1e-6f;
}

bool Transform2D::preservesAxisAlignment() const {
    // True if the transform only involves 90-degree rotations, scales, and translations
    float a = matrix.get(0, 0), b = matrix.get(0, 1);
    float c = matrix.get(1, 0), d = matrix.get(1, 1);

    return (std::abs(a) < 1e-6f && std::abs(d) < 1e-6f) ||
           (std::abs(b) < 1e-6f && std::abs(c) < 1e-6f);
}

float Transform2D::determinant() const {
    return matrix.get(0, 0) * matrix.get(1, 1) - matrix.get(0, 1) * matrix.get(1, 0);
}

float Transform2D::extractRotation() const {
    return std::atan2(matrix.get(1, 0), matrix.get(0, 0));
}

Vector2 Transform2D::extractTranslation() const {
    return Vector2(matrix.get(0, 2), matrix.get(1, 2));
}

Vector2 Transform2D::extractScale() const {
    float sx = std::sqrt(matrix.get(0, 0) * matrix.get(0, 0) +
                         matrix.get(1, 0) * matrix.get(1, 0));
    float sy = std::sqrt(matrix.get(0, 1) * matrix.get(0, 1) +
                         matrix.get(1, 1) * matrix.get(1, 1));

    // If determinant is negative, one axis is flipped
    if (determinant() < 0) sx = -sx;

    return Vector2(sx, sy);
}

bool Transform2D::decompose(Vector2& outTranslation, float& outRotation,
                              Vector2& outScale, float& outSkewX) const {
    float a = matrix.get(0, 0), b = matrix.get(0, 1);
    float c = matrix.get(1, 0), d = matrix.get(1, 1);
    float e = matrix.get(0, 2), f = matrix.get(1, 2);

    float det = a * d - b * c;
    if (std::abs(det) < 1e-12f) return false;

    outTranslation = Vector2(e, f);

    // Compute scale
    float sx = std::sqrt(a * a + c * c);
    float sy = std::sqrt(b * b + d * d);
    if (det < 0) sx = -sx;

    // Normalize
    float na = a / sx, nc = c / sx;
    float nb = b / sy, nd = d / sy;

    outRotation = std::atan2(nc, na);
    outScale = Vector2(sx, sy);

    // Skew: compute tan(skewX) = (na*nb + nc*nd)
    outSkewX = std::atan(na * nb + nc * nd);

    return true;
}

Transform2D Transform2D::interpolate(const Transform2D& from, const Transform2D& to, float t) {
    // Decompose both transforms and interpolate components
    Vector2 fromTrans, toTrans, fromScale, toScale;
    float fromRot, toRot, fromSkew, toSkew;

    if (!from.decompose(fromTrans, fromRot, fromScale, fromSkew) ||
        !to.decompose(toTrans, toRot, toScale, toSkew)) {
        // Fallback: per-element interpolation
        Transform2D result;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                float val = from.matrix.get(r, c) * (1.0f - t) + to.matrix.get(r, c) * t;
                result.matrix.set(r, c, val);
            }
        }
        return result;
    }

    // Interpolate each component
    float tx = fromTrans.x + (toTrans.x - fromTrans.x) * t;
    float ty = fromTrans.y + (toTrans.y - fromTrans.y) * t;
    float sx = fromScale.x + (toScale.x - fromScale.x) * t;
    float sy = fromScale.y + (toScale.y - fromScale.y) * t;

    // Shortest-path rotation
    float rot = toRot - fromRot;
    if (rot > static_cast<float>(M_PI)) rot -= 2.0f * static_cast<float>(M_PI);
    if (rot < -static_cast<float>(M_PI)) rot += 2.0f * static_cast<float>(M_PI);
    float angle = fromRot + rot * t;

    float sk = fromSkew + (toSkew - fromSkew) * t;

    // Recompose
    Transform2D result = translation(tx, ty) * rotation(angle) * skewX(sk) * scale(sx, sy);
    return result;
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

void Transform2D::skewSelf(float ax, float ay) {
    *this = skew(ax, ay) * (*this);
}

void Transform2D::preTranslate(float x, float y) {
    *this = (*this) * translation(x, y);
}

void Transform2D::preRotate(float angle) {
    *this = (*this) * rotation(angle);
}

void Transform2D::preScale(float sx, float sy) {
    *this = (*this) * scale(sx, sy);
}

} // namespace Math
} // namespace NXRender
