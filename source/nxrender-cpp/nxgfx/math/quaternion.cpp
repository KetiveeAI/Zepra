// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "quaternion.h"
#include <cmath>

namespace NXRender {
namespace Math {

Quaternion Quaternion::identity() { return Quaternion(0, 0, 0, 1); }

Quaternion Quaternion::fromAxisAngle(const Vector3& axis, float angle) {
    float halfAngle = angle * 0.5f;
    float s = std::sin(halfAngle);
    return Quaternion(axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle));
}

Quaternion Quaternion::fromEuler(float roll, float pitch, float yaw) {
    float cy = std::cos(yaw * 0.5f);
    float sy = std::sin(yaw * 0.5f);
    float cp = std::cos(pitch * 0.5f);
    float sp = std::sin(pitch * 0.5f);
    float cr = std::cos(roll * 0.5f);
    float sr = std::sin(roll * 0.5f);

    return Quaternion(
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
    );
}

Quaternion Quaternion::operator*(const Quaternion& q) const {
    return Quaternion(
        w * q.x + x * q.w + y * q.z - z * q.y,
        w * q.y - x * q.z + y * q.w + z * q.x,
        w * q.z + x * q.y - y * q.x + z * q.w,
        w * q.w - x * q.x - y * q.y - z * q.z
    );
}

Vector3 Quaternion::operator*(const Vector3& v) const {
    Vector3 qv(x, y, z);
    Vector3 u = qv.cross(v);
    Vector3 uu = qv.cross(u);
    return v + u * (2.0f * w) + uu * 2.0f;
}

Quaternion Quaternion::normalized() const {
    float len = std::sqrt(x*x + y*y + z*z + w*w);
    if (len > 0.0f) return Quaternion(x/len, y/len, z/len, w/len);
    return identity();
}

Quaternion Quaternion::conjugate() const {
    return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::inverse() const {
    float sqMag = x*x + y*y + z*z + w*w;
    if (sqMag > 0.0f) {
        Quaternion conj = conjugate();
        return Quaternion(conj.x / sqMag, conj.y / sqMag, conj.z / sqMag, conj.w / sqMag);
    }
    return identity();
}

Matrix4x4 Quaternion::toMatrix() const {
    Matrix4x4 m = Matrix4x4::identity();
    float xx = x * x, xy = x * y, xz = x * z, xw = x * w;
    float yy = y * y, yz = y * z, yw = y * w;
    float zz = z * z, zw = z * w;

    m.set(0, 0, 1 - 2 * (yy + zz));
    m.set(0, 1, 2 * (xy - zw));
    m.set(0, 2, 2 * (xz + yw));

    m.set(1, 0, 2 * (xy + zw));
    m.set(1, 1, 1 - 2 * (xx + zz));
    m.set(1, 2, 2 * (yz - xw));

    m.set(2, 0, 2 * (xz - yw));
    m.set(2, 1, 2 * (yz + xw));
    m.set(2, 2, 1 - 2 * (xx + yy));

    return m;
}

} // namespace Math
} // namespace NXRender
