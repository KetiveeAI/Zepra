// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "matrix.h"
#include "../primitives.h"

namespace NXRender {
namespace Math {

struct Transform2D {
    Matrix3x3 matrix;

    Transform2D();
    explicit Transform2D(const Matrix3x3& m);

    // Factory constructors
    static Transform2D identity();
    static Transform2D translation(float x, float y);
    static Transform2D rotation(float angleRadians);
    static Transform2D scale(float sx, float sy);
    static Transform2D skewX(float angleRadians);
    static Transform2D skewY(float angleRadians);
    static Transform2D skew(float angleX, float angleY);

    // CSS matrix(a, b, c, d, e, f)
    static Transform2D fromCSS(float a, float b, float c, float d, float e, float f);

    // Rotation around a point
    static Transform2D rotationAround(float angle, float cx, float cy);
    static Transform2D scaleAround(float sx, float sy, float cx, float cy);

    // Interpolation (for CSS transitions/animations)
    static Transform2D interpolate(const Transform2D& from, const Transform2D& to, float t);

    // Operators
    Transform2D operator*(const Transform2D& other) const;
    bool operator==(const Transform2D& other) const;

    // Point/rect mapping
    Vector2 mapPoint(const Vector2& point) const;
    void mapPoints(const Vector2* src, Vector2* dst, size_t count) const;
    Rect mapRect(const Rect& rect) const;

    // Inverse
    Transform2D inverse() const;

    // Queries
    bool isIdentity() const;
    bool isTranslation() const;
    bool isScaleTranslation() const;
    bool preservesAxisAlignment() const;
    float determinant() const;

    // Decomposition
    float extractRotation() const;
    Vector2 extractTranslation() const;
    Vector2 extractScale() const;
    bool decompose(Vector2& outTranslation, float& outRotation,
                   Vector2& outScale, float& outSkewX) const;

    // In-place mutation (pre-multiplied)
    void translate(float x, float y);
    void rotate(float angleRadians);
    void scaleSelf(float sx, float sy);
    void skewSelf(float ax, float ay);

    // Post-multiplied
    void preTranslate(float x, float y);
    void preRotate(float angle);
    void preScale(float sx, float sy);
};

} // namespace Math
} // namespace NXRender
