// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "matrix.h"
#include <cmath>

namespace NXRender {
namespace Math {

Matrix3x3::Matrix3x3() { m.fill(0); }

Matrix3x3 Matrix3x3::identity() {
    Matrix3x3 mat;
    mat.set(0, 0, 1);
    mat.set(1, 1, 1);
    mat.set(2, 2, 1);
    return mat;
}

Matrix3x3 Matrix3x3::operator*(const Matrix3x3& o) const {
    Matrix3x3 res;
    for(int r=0; r<3; r++) {
        for(int c=0; c<3; c++) {
            float sum = 0;
            for(int k=0; k<3; k++) sum += get(r, k) * o.get(k, c);
            res.set(r, c, sum);
        }
    }
    return res;
}

Vector2 Matrix3x3::operator*(const Vector2& v) const {
    float x = get(0,0)*v.x + get(0,1)*v.y + get(0,2)*1.0f;
    float y = get(1,0)*v.x + get(1,1)*v.y + get(1,2)*1.0f;
    float w = get(2,0)*v.x + get(2,1)*v.y + get(2,2)*1.0f;
    if (w != 0.0f && w != 1.0f && std::abs(w) > 1e-6f) {
        x /= w; y /= w;
    }
    return Vector2(x, y);
}

float Matrix3x3::determinant() const {
    return get(0,0) * (get(1,1) * get(2,2) - get(1,2) * get(2,1)) -
           get(0,1) * (get(1,0) * get(2,2) - get(1,2) * get(2,0)) +
           get(0,2) * (get(1,0) * get(2,1) - get(1,1) * get(2,0));
}

Matrix3x3 Matrix3x3::inverse() const {
    float det = determinant();
    if (std::abs(det) < 1e-6f) {
        return identity(); 
    }
    
    float invDet = 1.0f / det;
    Matrix3x3 res;
    res.set(0, 0,  (get(1,1) * get(2,2) - get(2,1) * get(1,2)) * invDet);
    res.set(0, 1, -(get(0,1) * get(2,2) - get(2,1) * get(0,2)) * invDet);
    res.set(0, 2,  (get(0,1) * get(1,2) - get(1,1) * get(0,2)) * invDet);
    
    res.set(1, 0, -(get(1,0) * get(2,2) - get(2,0) * get(1,2)) * invDet);
    res.set(1, 1,  (get(0,0) * get(2,2) - get(2,0) * get(0,2)) * invDet);
    res.set(1, 2, -(get(0,0) * get(1,2) - get(1,0) * get(0,2)) * invDet);
    
    res.set(2, 0,  (get(1,0) * get(2,1) - get(2,0) * get(1,1)) * invDet);
    res.set(2, 1, -(get(0,0) * get(2,1) - get(2,0) * get(0,1)) * invDet);
    res.set(2, 2,  (get(0,0) * get(1,1) - get(1,0) * get(0,1)) * invDet);
    
    return res;
}

// --- Matrix4x4 ---

Matrix4x4::Matrix4x4() { m.fill(0); }

Matrix4x4 Matrix4x4::identity() {
    Matrix4x4 mat;
    mat.set(0, 0, 1); mat.set(1, 1, 1); mat.set(2, 2, 1); mat.set(3, 3, 1);
    return mat;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& o) const {
    Matrix4x4 res;
    for(int r=0; r<4; r++) {
        for(int c=0; c<4; c++) {
            float sum = 0;
            for(int k=0; k<4; k++) sum += get(r, k) * o.get(k, c);
            res.set(r, c, sum);
        }
    }
    return res;
}

Vector4 Matrix4x4::operator*(const Vector4& v) const {
    return Vector4(
        get(0,0)*v.x + get(0,1)*v.y + get(0,2)*v.z + get(0,3)*v.w,
        get(1,0)*v.x + get(1,1)*v.y + get(1,2)*v.z + get(1,3)*v.w,
        get(2,0)*v.x + get(2,1)*v.y + get(2,2)*v.z + get(2,3)*v.w,
        get(3,0)*v.x + get(3,1)*v.y + get(3,2)*v.z + get(3,3)*v.w
    );
}

Vector3 Matrix4x4::multiplyPoint(const Vector3& v) const {
    Vector4 r = (*this) * Vector4(v.x, v.y, v.z, 1.0f);
    if(r.w != 1.0f && r.w != 0.0f && std::abs(r.w) > 1e-6f) {
        return Vector3(r.x/r.w, r.y/r.w, r.z/r.w);
    }
    return Vector3(r.x, r.y, r.z);
}

Vector3 Matrix4x4::multiplyDirection(const Vector3& v) const {
    Vector4 r = (*this) * Vector4(v.x, v.y, v.z, 0.0f);
    return Vector3(r.x, r.y, r.z);
}

Matrix4x4 Matrix4x4::inverse() const {
    Matrix4x4 inv;
    float t[6];
    float a = get(0,0), b = get(0,1), c = get(0,2), d = get(0,3);
    float e = get(1,0), f = get(1,1), g = get(1,2), h = get(1,3);
    float i = get(2,0), j = get(2,1), k = get(2,2), l = get(2,3);
    float m_ = get(3,0), n = get(3,1), o = get(3,2), p = get(3,3);

    t[0] = k * p - o * l; t[1] = j * p - n * l; t[2] = j * o - n * k;
    t[3] = i * p - m_ * l; t[4] = i * o - m_ * k; t[5] = i * n - m_ * j;

    inv.set(0, 0,  f * t[0] - g * t[1] + h * t[2]);
    inv.set(1, 0,-(e * t[0] - g * t[3] + h * t[4]));
    inv.set(2, 0,  e * t[1] - f * t[3] + h * t[5]);
    inv.set(3, 0,-(e * t[2] - f * t[4] + g * t[5]));

    inv.set(0, 1,-(b * t[0] - c * t[1] + d * t[2]));
    inv.set(1, 1,  a * t[0] - c * t[3] + d * t[4]);
    inv.set(2, 1,-(a * t[1] - b * t[3] + d * t[5]));
    inv.set(3, 1,  a * t[2] - b * t[4] + c * t[5]);

    t[0] = g * p - o * h; t[1] = f * p - n * h; t[2] = f * o - n * g;
    t[3] = e * p - m_ * h; t[4] = e * o - m_ * g; t[5] = e * n - m_ * f;

    inv.set(0, 2,  b * t[0] - c * t[1] + d * t[2]);
    inv.set(1, 2,-(a * t[0] - c * t[3] + d * t[4]));
    inv.set(2, 2,  a * t[1] - b * t[3] + d * t[5]);
    inv.set(3, 2,-(a * t[2] - b * t[4] + c * t[5]));

    t[0] = g * l - k * h; t[1] = f * l - j * h; t[2] = f * k - j * g;
    t[3] = e * l - i * h; t[4] = e * k - i * g; t[5] = e * j - i * f;

    inv.set(0, 3,-(b * t[0] - c * t[1] + d * t[2]));
    inv.set(1, 3,  a * t[0] - c * t[3] + d * t[4]);
    inv.set(2, 3,-(a * t[1] - b * t[3] + d * t[5]));
    inv.set(3, 3,  a * t[2] - b * t[4] + c * t[5]);

    float det = a * inv.get(0,0) + b * inv.get(1,0) + c * inv.get(2,0) + d * inv.get(3,0);
    if (std::abs(det) < 1e-6f) return identity();

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) inv.set(row, col, inv.get(row, col) / det);
    }
    return inv;
}

float Matrix4x4::determinant() const {
    float a = get(0,0), b = get(0,1), c = get(0,2), d = get(0,3);
    float e = get(1,0), f = get(1,1), g = get(1,2), h = get(1,3);
    float i = get(2,0), j = get(2,1), k = get(2,2), l = get(2,3);
    float m_ = get(3,0), n = get(3,1), o = get(3,2), p = get(3,3);

    float t0 = k * p - o * l;
    float t1 = j * p - n * l;
    float t2 = j * o - n * k;
    float t3 = i * p - m_ * l;
    float t4 = i * o - m_ * k;
    float t5 = i * n - m_ * j;

    float inv00 =  f * t0 - g * t1 + h * t2;
    float inv10 =-(e * t0 - g * t3 + h * t4);
    float inv20 =  e * t1 - f * t3 + h * t5;
    float inv30 =-(e * t2 - f * t4 + g * t5);

    return a * inv00 + b * inv10 + c * inv20 + d * inv30;
}

Matrix4x4 Matrix4x4::ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    Matrix4x4 mat;
    mat.set(0, 0, 2.0f / (right - left));
    mat.set(1, 1, 2.0f / (top - bottom));
    mat.set(2, 2, -2.0f / (zFar - zNear));
    mat.set(3, 3, 1.0f);
    mat.set(0, 3, -(right + left) / (right - left));
    mat.set(1, 3, -(top + bottom) / (top - bottom));
    mat.set(2, 3, -(zFar + zNear) / (zFar - zNear));
    return mat;
}

Matrix4x4 Matrix4x4::perspective(float fovY, float aspect, float zNear, float zFar) {
    Matrix4x4 mat;
    float f = 1.0f / std::tan(fovY / 2.0f);
    mat.set(0, 0, f / aspect);
    mat.set(1, 1, f);
    mat.set(2, 2, (zFar + zNear) / (zNear - zFar));
    mat.set(2, 3, (2.0f * zFar * zNear) / (zNear - zFar));
    mat.set(3, 2, -1.0f);
    return mat;
}

Matrix4x4 Matrix4x4::lookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    Vector3 f = (center - eye).normalized();
    Vector3 s = f.cross(up).normalized();
    Vector3 u = s.cross(f);
    
    Matrix4x4 mat = identity();
    mat.set(0, 0, s.x); mat.set(0, 1, s.y); mat.set(0, 2, s.z);
    mat.set(1, 0, u.x); mat.set(1, 1, u.y); mat.set(1, 2, u.z);
    mat.set(2, 0, -f.x); mat.set(2, 1, -f.y); mat.set(2, 2, -f.z);
    
    mat.set(0, 3, -s.dot(eye));
    mat.set(1, 3, -u.dot(eye));
    mat.set(2, 3, f.dot(eye));
    
    return mat;
}

} // namespace Math
} // namespace NXRender
