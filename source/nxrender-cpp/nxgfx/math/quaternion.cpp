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

Quaternion Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t) {
    // Dot product = cosine of angle between quaternions
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    // Take the shorter arc
    Quaternion end = b;
    if (dot < 0.0f) {
        dot = -dot;
        end = Quaternion(-b.x, -b.y, -b.z, -b.w);
    }

    // If quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        Quaternion result(
            a.x + t * (end.x - a.x),
            a.y + t * (end.y - a.y),
            a.z + t * (end.z - a.z),
            a.w + t * (end.w - a.w)
        );
        return result.normalized();
    }

    float theta0 = std::acos(dot);
    float theta = theta0 * t;
    float sinTheta0 = std::sin(theta0);
    float sinTheta = std::sin(theta);

    float s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
    float s1 = sinTheta / sinTheta0;

    return Quaternion(
        s0 * a.x + s1 * end.x,
        s0 * a.y + s1 * end.y,
        s0 * a.z + s1 * end.z,
        s0 * a.w + s1 * end.w
    ).normalized();
}

Quaternion Quaternion::fromMatrix(const Matrix4x4& m) {
    // Shepperd's method for rotation matrix -> quaternion
    float trace = m.get(0, 0) + m.get(1, 1) + m.get(2, 2);
    float x, y, z, w;

    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        w = 0.25f / s;
        x = (m.get(2, 1) - m.get(1, 2)) * s;
        y = (m.get(0, 2) - m.get(2, 0)) * s;
        z = (m.get(1, 0) - m.get(0, 1)) * s;
    } else if (m.get(0, 0) > m.get(1, 1) && m.get(0, 0) > m.get(2, 2)) {
        float s = 2.0f * std::sqrt(1.0f + m.get(0, 0) - m.get(1, 1) - m.get(2, 2));
        w = (m.get(2, 1) - m.get(1, 2)) / s;
        x = 0.25f * s;
        y = (m.get(0, 1) + m.get(1, 0)) / s;
        z = (m.get(0, 2) + m.get(2, 0)) / s;
    } else if (m.get(1, 1) > m.get(2, 2)) {
        float s = 2.0f * std::sqrt(1.0f + m.get(1, 1) - m.get(0, 0) - m.get(2, 2));
        w = (m.get(0, 2) - m.get(2, 0)) / s;
        x = (m.get(0, 1) + m.get(1, 0)) / s;
        y = 0.25f * s;
        z = (m.get(1, 2) + m.get(2, 1)) / s;
    } else {
        float s = 2.0f * std::sqrt(1.0f + m.get(2, 2) - m.get(0, 0) - m.get(1, 1));
        w = (m.get(1, 0) - m.get(0, 1)) / s;
        x = (m.get(0, 2) + m.get(2, 0)) / s;
        y = (m.get(1, 2) + m.get(2, 1)) / s;
        z = 0.25f * s;
    }

    return Quaternion(x, y, z, w).normalized();
}

} // namespace Math
} // namespace NXRender
