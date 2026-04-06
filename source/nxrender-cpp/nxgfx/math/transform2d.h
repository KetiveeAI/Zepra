// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "matrix.h"

namespace NXRender {
namespace Math {

struct Transform2D {
    Matrix3x3 matrix;

    Transform2D();
    explicit Transform2D(const Matrix3x3& m);

    static Transform2D identity();
    static Transform2D translation(float x, float y);
    static Transform2D rotation(float angleRadians);
    static Transform2D scale(float sx, float sy);

    Transform2D operator*(const Transform2D& other) const;
    Vector2 mapPoint(const Vector2& point) const;

    Transform2D inverse() const;
    void translate(float x, float y);
    void rotate(float angleRadians);
    void scaleSelf(float sx, float sy);
};

} // namespace Math
} // namespace NXRender
