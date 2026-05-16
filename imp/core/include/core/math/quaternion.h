#pragma once

#include <core/math/vec3.h>
#include <core/math/vec4.h>
#include <core/math/mat3.h>
#include <core/math/mat4.h>

#include <cmath>
#include <cassert>
#include <type_traits>

namespace imp::math
{
    template <typename T>
    struct Quaternion
    {
        static_assert(std::is_floating_point_v<T>, "Quaternion<T> requires a floating-point type");

        T x, y, z, w;

        // Identity
        constexpr Quaternion() noexcept : x(T(0)), y(T(0)), z(T(0)), w(T(1)) {}
        constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

        // Build from a normalised axis and an angle (radians)
        [[nodiscard]] static inline Quaternion fromAxisAngle(const Vec3<T>& axis, T angle) noexcept
        {
            const Vec3<T> a = normalise(axis);

            const T half = angle * T(0.5);
            const T s = std::sin(half);
            return { a.x * s, a.y * s, a.z * s, std::cos(half) };
        }

        [[nodiscard]] static inline Quaternion fromEuler(T pitch, T yaw, T roll) noexcept
        {
            const T hp = pitch * T(0.5), hy = yaw * T(0.5), hr = roll * T(0.5);
            const T cp = std::cos(hp), sp = std::sin(hp);
            const T cy = std::cos(hy), sy = std::sin(hy);
            const T cr = std::cos(hr), sr = std::sin(hr);

            return
            {
                cy*sp*cr + sy*cp*sr,
                sy*cp*cr - cy*sp*sr,
                cy*cp*sr - sy*sp*cr,
                cy*cp*cr + sy*sp*sr,
            };
        }

        // Build from a column-major rotation matrix (must be pure rotation, no scale)
        [[nodiscard]] static inline Quaternion fromMat3(const Mat3<T>& m) noexcept
        {
            const T trace = m(0,0) + m(1,1) + m(2,2);
            Quaternion q;

            if (trace > T(0))
            {
                // w is largest
                const T s = std::sqrt(trace + T(1)) * T(2);   // s = 4w
                q.w = T(0.25) * s;
                q.x = (m(2,1) - m(1,2)) / s;
                q.y = (m(0,2) - m(2,0)) / s;
                q.z = (m(1,0) - m(0,1)) / s;
            }
            else if (m(0,0) > m(1,1) && m(0,0) > m(2,2))
            {
                // x is largest
                const T s = std::sqrt(T(1) + m(0,0) - m(1,1) - m(2,2)) * T(2);   // s = 4x
                q.w = (m(2,1) - m(1,2)) / s;
                q.x = T(0.25) * s;
                q.y = (m(0,1) + m(1,0)) / s;
                q.z = (m(0,2) + m(2,0)) / s;
            }
            else if (m(1,1) > m(2,2))
            {
                // y is largest
                const T s = std::sqrt(T(1) + m(1,1) - m(0,0) - m(2,2)) * T(2);   // s = 4y
                q.w = (m(0,2) - m(2,0)) / s;
                q.x = (m(0,1) + m(1,0)) / s;
                q.y = T(0.25) * s;
                q.z = (m(1,2) + m(2,1)) / s;
            }
            else
            {
                // z is largest
                const T s = std::sqrt(T(1) + m(2,2) - m(0,0) - m(1,1)) * T(2);   // s = 4z
                q.w = (m(1,0) - m(0,1)) / s;
                q.x = (m(0,2) + m(2,0)) / s;
                q.y = (m(1,2) + m(2,1)) / s;
                q.z = T(0.25) * s;
            }

            return q;
        }

        static inline Quaternion fromMat4(const Mat4<T>& m) noexcept
        {
            return fromMat3(Mat3<T>{m});
        }

        // Constants
        [[nodiscard]] static constexpr Quaternion identity() noexcept { return {}; }

        // Array accessor
        [[nodiscard]] constexpr T& operator[](int i) noexcept
        {
            assert(i >= 0 && i < 4);
            return (&x)[i];
        }
        [[nodiscard]] constexpr const T& operator[](int i) const noexcept
        {
            assert(i >= 0 && i < 4);
            return (&x)[i];
        }

        // Conversion to Vec4 storage
        [[nodiscard]] constexpr Vec4<T> toVec4() const noexcept { return {x, y, z, w}; }

        // Member arithmetic
        constexpr Quaternion operator-() const noexcept { return {-x, -y, -z, -w}; }

        constexpr Quaternion& operator+=(const Quaternion& rhs) noexcept { x+=rhs.x; y+=rhs.y; z+=rhs.z; w+=rhs.w; return *this; }
        constexpr Quaternion& operator-=(const Quaternion& rhs) noexcept { x-=rhs.x; y-=rhs.y; z-=rhs.z; w-=rhs.w; return *this;}
        constexpr Quaternion& operator*=(T s) noexcept { x*=s; y*=s; z*=s; w*=s; return *this; }

        // Hamilton product (compose rotations): q *= r means first r then q
        constexpr Quaternion& operator*=(const Quaternion& rhs) noexcept
        {
            *this = *this * rhs;
            return *this;
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const Quaternion& rhs) const noexcept
        {
            return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
        }
        [[nodiscard]] constexpr bool operator!=(const Quaternion& rhs) noexcept
        {
            return !(*this == rhs);
        }
    };

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator+(Quaternion<T> a, const Quaternion<T>& b) noexcept { a += b; return a; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator-(Quaternion<T> a, const Quaternion<T>& b) noexcept { a -= b; return a; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator*(Quaternion<T> q, T s) noexcept { q *= s; return q; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator*(T s, Quaternion<T> q) noexcept { q *= s; return q; }

    // Hamilton product: p * q - applies q's rotation first, then p's
    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator*(const Quaternion<T>& p, const Quaternion<T>& q) noexcept
    {
        return {
          p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
            p.w * q.y - p.x * q.z + p.y * q.w + p.z * q.x,
            p.w * q.z + p.x * q.y - p.y * q.x + p.z * q.w,
            p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
        };
    }

    // Core operations
    template <typename T>
    [[nodiscard]] constexpr T dot(const Quaternion<T>& a, const Quaternion<T>& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    template <typename T>
    [[nodiscard]] inline T length(const Quaternion<T>& q) noexcept
    {
        return std::sqrt(dot(q, q));
    }

    template <typename T>
    [[nodiscard]] inline Quaternion<T> normalise(const Quaternion<T>& q) noexcept
    {
        const T len = length(q);
        assert(len > T(0) && "Normalised called on a zero-length Quaternion");
        return q * (T(1) / len);
    }

    // Conjugate: same rotation axis, negated angle - also the inverse for unit quaternion
    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> conjugate(const Quaternion<T>& q) noexcept
    {
        return { -q.x, -q.y, -q.z, q.w };
    }

    // Inverse: valid for non-unit quaternions too (divide conjugate by squared length)
    template <typename T>
    [[nodiscard]] inline Quaternion<T> inverse(const Quaternion<T>& q) noexcept
    {
        const T lenSq = dot(q, q);
        assert(lenSq > T(0) && "Inverse called on zero-length Quaternion");
        const T inv = T(1) / lenSq;
        return { -q.x * inv, -q.y * inv, -q.z * inv, q.w * inv };
    }

    // Rotation application
    // Rotate a vector by a unit quaternion: v' = q * v * q^{-1}
    template <typename T>
    [[nodiscard]] constexpr Vec3<T> rotate(const Quaternion<T>& q, const Vec3<T>& v) noexcept
    {
        // Efficient sandwich product via t = 2 * (q.xyz * v)
        const Vec3<T> qv = { q.x, q.y, q.z };
        const Vec3<T> t = cross(qv, v) * T(2);
        return v + t * q.w + cross(qv, t);
    }

    // Conversion - to matrix
    template <typename T>
    [[nodiscard]] constexpr Mat3<T> toMat3(const Quaternion<T>& q) noexcept
    {
        const T xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        const T xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        const T wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        return Mat3<T>
        {
            Vec3<T> { T(1) - T(2) * (yy+zz), T(2) * (xy + wz), T(2) * (xz - wy) },
            Vec3<T> { T(2) * (xy - wz), T(1) - T(2) * (xx + zz), T(2) * (yz + wx) },
            Vec3<T> { T(2) * (xz + wy), T(2) * (yz - wx), T(1) - T(2) * (xx + yy) },
        };
    }

    template <typename T>
    [[nodiscard]] constexpr Mat4<T> toMat4(const Quaternion<T>& q) noexcept
    {
        return toMat4(toMat3(q));
    }

    // Conversion - To Euler (YXZ, radians)
    template <typename T>
    struct EulerAngles { T pitch, yaw, roll; };

    template <typename T>
    [[nodiscard]] inline EulerAngles<T> toEuler(const Quaternion<T>& q) noexcept
    {
        EulerAngles<T> e;

        // Pitch  (X-axis)
        const T sinp = T(2) * (q.w*q.x + q.y*q.z);
        const T cosp = T(1) - T(2) * (q.x*q.x + q.y*q.y);
        e.pitch = std::atan2(sinp, cosp);

        // Yaw  (Y-axis)
        const T siny = T(2) * (q.w*q.y - q.z*q.x);
        const T clampedSiny = siny < T(-1) ? T(-1) : (siny > T(1) ? T(1) : siny);
        e.yaw = std::asin(clampedSiny);

        // Roll  (Z-axis)
        const T sinr = T(2) * (q.w*q.z + q.x*q.y);
        const T cosr = T(1) - T(2) * (q.y*q.y + q.z*q.z);
        e.roll = std::atan2(sinr, cosr);

        return e;
    }

    // Interpolation
    // Normalised linear interpolation - fast, slight speed variation
    template <typename T>
    [[nodiscard]] inline Quaternion<T> nlerp(const Quaternion<T>& a, const Quaternion<T>& b, T t) noexcept
    {
        // Ensure shortest path
        const Quaternion<T> b2 = dot(a, b) < T(0) ? -b : b;
        return normalise(a + (b2 - a) * t);
    }

    // Spherical linear interpolation - constant angular velocity
    template <typename T>
    [[nodiscard]] inline Quaternion<T> slerp(const Quaternion<T>& a, const Quaternion<T>& b, T t) noexcept
    {
        constexpr T kEpsilon = T(1e-6);

        T cosAngle = dot(a, b);
        Quaternion<T> b2 = b;

        if (cosAngle < T(0)) { cosAngle = -cosAngle; b2 = -b; }
        if (cosAngle > T(1) - kEpsilon)
        {
            return nlerp(a, b2, t); // nearly identical - fallback to nlerp
        }

        const T angle = std::acos(cosAngle);
        const T sinAngle = std::sin(angle);

        return a * (std::sin((T(1) - t) * angle) / sinAngle)
            + b2 * (std::sin(t * angle) / sinAngle);
    }

    // Type defs

    using Quaternionf = Quaternion<float>;
    using Quaterniond = Quaternion<double>;
}
