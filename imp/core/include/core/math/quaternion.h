#pragma once

#include <core/math/vec3.h>
#include <core/math/vec4.h>
#include <core/math/mat3.h>
#include <core/math/mat4.h>

#include <cmath>
#include <cassert>
#include <iterator>
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
        static inline Quaternion fromAxisAngle(const Vec3<T>& axis, T angle) noexcept
        {
            const T half = angle * T(0.5);
            const T s = std::sin(half);
            return { axis.x * s, axis.y * s, axis.z * s, std::cos(half) };
        }

        // Build from Euler angles (radians) applied in YXZ order (yaw-pitch-roll)
        // yaw = about Y, pitch = about X, roll = about Z.
        static inline Quaternion fromEuler(T pitch, T yaw, T roll) noexcept
        {
            const T hp = pitch * T(0.5), hy = yaw * T(0.5), hr = roll * T(0.5);
            const T cp = std::cos(hp), sp = std::sin(hp);
            const T cy = std::cos(cy), sy = std::sin(cy);
            const T cr = std::cos(cr), sr = std::sin(cr);

            return {
                cy*sp*cr + sy*cp*sr,
                sy*cp*cr - cy*sp*sr,
                cy*cp*sr - sy*sp*cr,
                cy*cp*cr + sy*sp*sr,
            };
        }

        // Build from a column-major rotation matrix (must be pure rotation, no scale)
        static inline Quaternion fromMat3(const Mat3<T>& m) noexcept
        {
            // Shepperd's method
            const T trace = m(0, 0) + m(1, 1) + m(2, 2);
            Quaternion q;
            if (trace > T(0))
            {
                const T s = std::sqrt(trace + T(1)) * T(2);
                q.w = T(0.25) * s;
                q.x = (m(2, 1) - m(1,2)) / s;
                q.y = (m(0, 2) - m(2,0)) / s;
                q.z = (m(1, 0) - m(0,1)) / s;
            }
            else if (m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2))
            {
                const T s = std::sqrt(T(1) + m(0, 0) - m(1, 1) - m(2, 2)) * T(2);
                q.w = (m(2, 1) - m(1,2)) / s;
                q.x = T(0.25) * s;
                q.y = (m(0, 1) + m(1, 0)) / s;
                q.z = (m(0, 2) + m(2, 0)) / s;
            }
            else if (m(1, 1) > m(2, 2))
            {
                const T s = std::sqrt(T(1) + m(1, 1) - m(0, 0) - m(2, 2)) * T(2);
                q.w = (m(0, 2) - m(2, 0)) / s;
                q.x = (m(0, 1) + m(1, 0)) / s;
                q.y = T(0.25) * s;
                q.z = (m(1, 2) + m(2, 1)) / s;
            }
            else
                {
                const T s = std::sqrt(T(1) + m(2, 2) - m(0, 0) - m(1, 1)) * T(2);
                q.w = (m(1, 0) - m(0, 1)) / s;
                q.x = (m(0, 2) + m(2, 0)) / s;
                q.y = (m(1, 2) + m(2, 1)) / s;
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

        constexpr Quaternion& operator+=(const Quaternion& rhs) const noexcept { x+=rhs.x; y+=rhs.y; z+=rhs.z; w+=rhs.w; return *this; }
        constexpr Quaternion& operator-=(const Quaternion& rhs) const noexcept { x-=rhs.x; y-=rhs.y; z-=rhs.z; w-=rhs.w; return *this;}
        constexpr Quaternion& operator*=(T s) noexcept { x*=s; y*=s; z*=s; w*=s; return *this; }

        // Hamilton product (compose rotations): q *= r means first r then q
        constexpr Quaternion& operator*=(const Quaternion& rhs) noexcept
        {
            *this = *this * rhs;
            return *this;
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const Quaternion& rhs) noexcept
        {
            return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
        }
        [[nodiscard]] constexpr bool operator!=(const Quaternion& rhs) noexcept
        {
            return !(*this == rhs);
        }
    };

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator+(Quaternion<T> a, const Quaternion<T>& b) noexcept { return a += b; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator-(Quaternion<T> a, const Quaternion<T>& b) noexcept { return a -= b; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator*(Quaternion<T> q, T s) noexcept { return q *= s; }

    template <typename T>
    [[nodiscard]] constexpr Quaternion<T> operator*(T s, Quaternion<T> q) noexcept { return q *= s; }

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
}
