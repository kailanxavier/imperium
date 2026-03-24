#pragma once

#include <core/math/vec3.h>
#include <core/math/mat4.h>

#include <cmath>
#include <cassert>
#include <type_traits>

namespace imp::math
{
    template <typename T>
    struct Mat3
    {
        static_assert(std::is_floating_point_v<T>, "Mat<T> requires a floating-point type");

        Vec3<T> col[3];

        // Construction
        // Identity
        constexpr Mat3() noexcept
            : col { Vec3<T>{T(1), T(0), T(0)},
                    Vec3<T>{T(0), T(1), T(0)},
                    Vec3<T>{T(0), T(0), T(1)}
            } {}

        constexpr Mat3(const Vec3<T>& c0, const Vec3<T>& c1, const Vec3<T>& c2) noexcept
            : col { c0, c1, c2 } {}

        constexpr explicit Mat3(T diag) noexcept
            : col { Vec3<T> { diag, T(0), T(0)},
                    Vec3<T> { T(0), diag, T(0)},
                    Vec3<T> { T(0), T(0), diag}
            } {}

        // Extract upper-left 3x3 from a Mat4 (strips translation)
        constexpr explicit Mat3(const Mat4<T>& m) noexcept
            : col { Vec3<T>{m(0, 0), m(1, 0), m(2, 0)},
                    Vec3<T>{m(0, 1), m(1, 1), m(2, 1)},
                    Vec3<T>{m(0, 2), m(1, 2), m(2, 2)}
            } {}

        template <typename U>
        constexpr explicit Mat3(const Mat3<U>& o) noexcept
            : col { Vec3<T>(o.col[0]), Vec3<T>(o.col[1]), Vec3<T>(o.col[2])} {}

        // Element access
        [[nodiscard]] constexpr Vec3<T>& operator[](int c) noexcept { assert(c >= 0 && c < 3); return col[c]; }
        [[nodiscard]] constexpr const Vec3<T>& operator[](int c) const noexcept { assert(c >= 0 && c < 3); return col[c]; }

        [[nodiscard]] constexpr T& operator()(int r, int c) noexcept { assert(r >= 0 && r < 3 && c >= 0 && c < 3); return col[c][r]; }
        [[nodiscard]] constexpr const T& operator()(int r, int c) const noexcept { assert(r >= 0 && r < 3 && c >= 0 && c < 3); return col[c][r]; }

        [[nodiscard]] constexpr const T* data() const noexcept { return &col[0].x; }
        [[nodiscard]] constexpr T* data() noexcept { return &col[0].x; }

        // Constants
        [[nodiscard]] static constexpr Mat3 identity() noexcept { return Mat3 {}; }
        [[nodiscard]] static constexpr Mat3 zero() noexcept { return Mat3 {T(0)}; }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const Mat3& rhs) const noexcept
        {
            return col[0] == rhs.col[0] && col[1] == rhs.col[1] && col[2] == rhs.col[2];
        }
        [[nodiscard]] constexpr bool operator!=(const Mat3& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };

    template <typename T>
    [[nodiscard]] constexpr Mat3<T> operator*(const Mat3<T>& a, const Mat3<T>& b) noexcept
    {
        Mat3<T> r;
        for (int c = 0; c < 3; c++)
        {
            r.col[c] = {
                a(0, 0) * b(0, c) + a(0, 1) * b(1, c) + a(0, 2) * b(2, c),
                a(1, 0) * b(0, c) + a(1, 1) * b(1, c) + a(1, 2) * b(2, c),
                a(2, 0) * b(0, c) + a(2, 1) * b(1, c) + a(2, 2) * b(2, c)
            };
        }
        return r;
    }

    template <typename T>
    [[nodiscard]] constexpr Vec3<T> operator*(const Mat3<T>& m, const Vec3<T>& v) noexcept
    {
        return {
            m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z,
            m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z,
            m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z
        };
    }

    // Transpose
    template <typename T>
    [[nodiscard]] constexpr Mat3<T> transpose(const Mat3<T>& m) noexcept
    {
        return {
            Vec3<T>{ m(0, 0), m(0, 1), m(0, 2) },
            Vec3<T>{ m(1, 0), m(1, 1), m(1, 2) },
            Vec3<T>{ m(2, 0), m(2, 1), m(2, 2) }
        };
    }

    // Determinant & Inverse
    template <typename T>
    [[nodiscard]] constexpr T determinant(const Mat3<T>& m) noexcept
    {
        return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1))
        - m(0,1) * (m(1,0)*m(2,2) - m(1,2)*m(2,0))
        + m(0,2) * (m(1,0)*m(2,1) - m(1,1)*m(2,0));
    }

    template <typename T>
    [[nodiscard]] inline Mat3<T> inverse(const Mat3<T>& m) noexcept
    {
        const T det = determinant(m);
        assert(det != T(0) && "Mat3::inverse - singular matrix");
        const T inv = T(1) / det;

        return Mat3<T>{
            Vec3<T>{
                (m(1,1) * m(2, 2) - m(1, 2) * m(2, 1)) * inv,
                (m(1, 2) * m(2,0) - m(1, 0) * m(2, 2) * inv),
                (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * inv
            },
            Vec3<T>{
                (m(0,2) * m(2,1) - m(0,1) * m(2,2)) * inv,
                (m(0,0) * m(2,2) - m(0,2) * m(2,0)) * inv,
                (m(0,1) * m(2,0) - m(0,0) * m(2,1)) * inv,
            },
            Vec3<T>{
                (m(0,1) * m(1,2) - m(0,2) * m(1,1)) * inv,
                (m(0,2) * m(1,0) - m(0,0) * m(1,2)) * inv,
                (m(0,0) * m(1,1) - m(0,1) * m(1,0)) * inv,
            },
        };
    }

    // Normal matrix
    // The normal matrix for a given model matrix M is: transpose(inverse(Mat3(M)))
    // Use this to transform surface normals; avoids shear distortion under non-uniform scale.

    template <typename T>
    [[nodiscard]] inline Mat3<T> makeNormalMatrix(const Mat4<T>& modelMatrix) noexcept
    {
        return transpose(inverse(Mat3<T>{modelMatrix}));
    }

    // Rotation factories
    template <typename T>
    [[nodiscard]] inline Mat3<T> makeRotation3(const Vec3<T>& axis, T angle) noexcept
    {
        const T c = std::cos(angle);
        const T s = std::sin(angle);
        const T t = T(1) - c;
        const T x = axis.x, y = axis.y, z = axis.z;

        return Mat3<T>{
          Vec3<T> { t*x*x+c, t*x*y+s*z, t*x*z-s*y },
            Vec3<T> { t*x*y-s*z, t*y*y+c, t*y*z+s*x },
            Vec3<T> { t*x*z+s*y, t*y*z-s*x, t*z*z+c },
        };
    }

    // Embed/extract
    // Embed a Mat3 into the upper-left of an identity Mat4
    template <typename T>
    [[nodiscard]] constexpr Mat4<T> toMat4(const Mat3<T>& m) noexcept
    {
        return Mat4<T> {
          Vec4<T>{ m(0, 0), m(1, 0), m(2, 0), T(0) },
            Vec4<T> { m(0, 1), m(1, 1), m(2, 1), T(0) },
            Vec4<T> { m(0, 2), m(1, 2), m(2, 2), T(0) },
            Vec4<T> {T(0), T(0), T(0), T(1) },
        };
    }

    // Type defs
    using Mat3f = Mat3<float>;
    using Mat3d = Mat3<double>;
}

