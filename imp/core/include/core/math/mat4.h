#pragma once

#include <core/math/vec3.h>
#include <core/math/vec4.h>

#include <cmath>
#include <cassert>
#include <type_traits>

namespace imp::math
{
	template <typename T>
	struct Mat4
	{
		static_assert( std::is_floating_point_v<T>, "Mat4<T> requires a floating-point type" );

		Vec4<T> col[4];

		// Identity
		constexpr Mat4() noexcept :
			col{ Vec4<T> { T(1), T(0), T(0), T(0) },
				 Vec4<T> { T(0), T(1), T(0), T(0) },
				 Vec4<T> { T(0), T(0), T(1), T(0) },
				 Vec4<T> { T(0), T(0), T(0), T(1) } } {
		}

		// Explicit column construction
		constexpr Mat4(const Vec4<T>& c0, const Vec4<T>& c1, const Vec4<T>& c2, const Vec4<T>& c3) noexcept :
			col{ c0, c1, c2, c3 } {
		}

		// Scalar diagonal
		constexpr explicit Mat4(T diag) noexcept :
			col{ Vec4<T> { diag, T(0), T(0), T(0) },
				 Vec4<T> { T(0), diag, T(0), T(0) },
				 Vec4<T> { T(0), T(0), diag, T(0) },
				 Vec4<T> { T(0), T(0), T(0), diag } } {
		}

		// Conversion between Mat4<float> and Mat4<double>
		template <typename U>
		constexpr explicit Mat4(const Mat4<U>& o) noexcept
			: col{ Vec4<T>(o.col[0]), Vec4<T>(o.col[1]), Vec4<T>(o.col[2]), Vec4<T>(o.col[3]) } {
		}

		// Element access
		[[nodiscard]] constexpr Vec4<T>& operator[](int c) noexcept
		{
			assert(c >= 0 && c < 4);
			return col[c];
		}
		[[nodiscard]] constexpr const Vec4<T>& operator[](int c) const noexcept
		{
			assert(c >= 0 && c < 4);
			return col[c];
		}

		// Named element access
		[[nodiscard]] constexpr T& operator()(int r, int c) noexcept
		{
			assert(r >= 0 && r < 4 && c >= 0 && c < 4);
			return col[c][r];
		}
		[[nodiscard]] constexpr const T& operator()(int r, int c) const noexcept
		{
			assert(r >= 0 && r < 4 && c >= 0 && c < 4);
			return col[c][r];
		}

		// Raw pointer for GPU upload - column major, ready for D3D/GL
		[[nodiscard]] constexpr const T* data() const noexcept { return &col[0].x; }
		[[nodiscard]] constexpr T* data() noexcept { return &col[0].x; }

		// Constants
		[[nodiscard]] static constexpr Mat4 identity() noexcept { return Mat4{}; }
		[[nodiscard]] static constexpr Mat4 zero() noexcept { return Mat4{ T(0) }; } // zero diagonal

		// Comparison
		[[nodiscard]] constexpr bool operator==(const Mat4& rhs) const noexcept
		{
			return col[0] == rhs.col[0] && col[1] == rhs.col[1] && col[2] == rhs.col[2] && col[3] == rhs.col[3];
		}
		[[nodiscard]] constexpr bool operator!=(const Mat4& rhs) const noexcept
		{
			return !( *this == rhs );
		}
	};

	template <typename T>
	[[nodiscard]] constexpr Mat4<T> operator*(const Mat4<T>& a, const Mat4<T>& b) noexcept
	{
		Mat4<T> r;
		for (int c = 0; c < 4; c++)
		{
			r.col[c] =
			{
				a(0, 0) * b(0, c) + a(0, 1) * b(1, c) + a(0,2) * b(2, c) + a(0, 3) * b(3, c),
				a(1, 0) * b(0, c) + a(1, 1) * b(1, c) + a(1,2) * b(2, c) + a(1, 3) * b(3, c),
				a(2, 0) * b(0, c) + a(2, 1) * b(1, c) + a(2,2) * b(2, c) + a(2, 3) * b(3, c),
				a(3, 0) * b(0, c) + a(3, 1) * b(1, c) + a(3,2) * b(2, c) + a(3, 3) * b(3, c)
			};
		}

		return r;
	}

	template <typename T>
	[[nodiscard]] constexpr Vec4<T> operator*(const Mat4<T>& m, const const Vec4<T>& v) noexcept
	{
		return
		{
			m(0, 0) * v.x + m(0, 1) * v.y + m(0,2) * v.z + m(0, 3) * v.w,
			m(1, 0) * v.x + m(1, 1) * v.y + m(1,2) * v.z + m(1, 3) * v.w,
			m(2, 0) * v.x + m(2, 1) * v.y + m(2,2) * v.z + m(2, 3) * v.w,
			m(3, 0) * v.x + m(3, 1) * v.y + m(3,2) * v.z + m(3, 3) * v.w
		};
	}

	// Transform a point (w = 1)
	template <typename T>
	[[nodiscard]] constexpr Vec3<T> transformPoint(const Mat4<T>& m, const Vec3<T> p) noexcept
	{
		const Vec4<T> r = m * Vec4<T>{ p.x, p.y, p.z, T(1) };
		return { r.x, r.y, r.z };
	}

	// Transform a direction (w = 0)
	template <typename T>
	[[nodiscard]] constexpr Vec3<T> transformDirection(const Mat4<T>& m, const Vec3<T> d) noexcept
	{
		const Vec4<T> r = m * Vec4<T>{ d.x, d.y, d.z, T(0) };
		return { r.x, r.y, r.z };
	}

	// Transpose
	template <typename T>
	[[nodiscard]] constexpr Mat4<T> transpose(const Mat4<T>& m) noexcept
	{
		return
		{
			Vec4<T> { m(0, 0), m(0, 1), m(0, 2), m(0, 3) },
			Vec4<T> { m(1, 0), m(1, 1), m(1, 2), m(1, 3) },
			Vec4<T> { m(2, 0), m(2, 1), m(2, 2), m(2, 3) },
			Vec4<T> { m(3, 0), m(3, 1), m(3, 2), m(3, 3) },
		};
	}

	// Inverse
	// Full 4x4 inverse via cofactor expansion.
	// Returns identity and asserts if the matrix is singular.

	template <typename T>
	[[nodiscard]] inline Mat4<T> inverse(const Mat4<T>& m) noexcept
	{
		// Notation: mRC (row, column)
		const T m00 = m(0, 0), m01 = m(0, 1), m02 = m(0, 2), m03 = m(0, 3);
		const T m10 = m(1, 0), m11 = m(1, 1), m12 = m(1, 2), m13 = m(1, 3);
		const T m20 = m(2, 0), m21 = m(2, 1), m22 = m(2, 2), m23 = m(2, 3);
		const T m30 = m(3, 0), m31 = m(3, 1), m32 = m(3, 2), m33 = m(3, 3);

		const T c00 = ( m11 * ( m22 * m33 - m23 * m32 ) - m12 * ( m21 * m33 - m23 * m31 ) + m13 * ( m21 * m32 - m22 * m31 ) );
		const T c01 = -( m10 * ( m22 * m33 - m23 * m32 ) - m12 * ( m20 * m33 - m23 * m30 ) + m13 * ( m20 * m32 - m22 * m30 ) );
		const T c02 = ( m10 * ( m21 * m33 - m23 * m31 ) - m11 * ( m20 * m33 - m23 * m30 ) + m13 * ( m20 * m31 - m21 * m30 ) );
		const T c03 = -( m10 * ( m21 * m32 - m22 * m31 ) - m11 * ( m20 * m32 - m22 * m30 ) + m13 * ( m20 * m31 - m21 * m30 ) );

		const T det = m00 * c00 + m01 * c01 + m02 * c02 + m03 * c03;
		assert(det != T(0) && "Mat4::inverse - singular matrix");

		const T invDet = T(1) / det;

		const T c10 = -( m01 * ( m22 * m33 - m23 * m32 ) - m02 * ( m21 * m33 - m23 * m31 ) + m03 * ( m21 * m32 - m22 * m31 ) );
		const T c11 = ( m00 * ( m22 * m33 - m23 * m32 ) - m02 * ( m20 * m33 - m23 * m30 ) + m03 * ( m20 * m32 - m22 * m30 ) );
		const T c12 = -( m00 * ( m21 * m33 - m23 * m31 ) - m01 * ( m20 * m33 - m23 * m30 ) + m03 * ( m20 * m31 - m21 * m30 ) );
		const T c13 = ( m00 * ( m21 * m32 - m22 * m31 ) - m01 * ( m20 * m32 - m22 * m30 ) + m02 * ( m20 * m31 - m21 * m30 ) );

		const T c20 = ( m01 * ( m12 * m33 - m13 * m32 ) - m02 * ( m11 * m33 - m13 * m31 ) + m03 * ( m11 * m32 - m12 * m31 ) );
		const T c21 = -( m00 * ( m12 * m33 - m13 * m32 ) - m02 * ( m10 * m33 - m13 * m30 ) + m03 * ( m10 * m32 - m12 * m30 ) );
		const T c22 = ( m00 * ( m11 * m33 - m13 * m31 ) - m01 * ( m10 * m33 - m13 * m30 ) + m03 * ( m10 * m31 - m11 * m30 ) );
		const T c23 = -( m00 * ( m11 * m32 - m12 * m31 ) - m01 * ( m10 * m32 - m12 * m30 ) + m02 * ( m10 * m31 - m11 * m30 ) );

		const T c30 = -( m01 * ( m12 * m23 - m13 * m22 ) - m02 * ( m11 * m23 - m13 * m21 ) + m03 * ( m11 * m22 - m12 * m21 ) );
		const T c31 = ( m00 * ( m12 * m23 - m13 * m22 ) - m02 * ( m10 * m23 - m13 * m20 ) + m03 * ( m10 * m22 - m12 * m20 ) );
		const T c32 = -( m00 * ( m11 * m23 - m13 * m21 ) - m01 * ( m10 * m23 - m13 * m20 ) + m03 * ( m10 * m21 - m11 * m20 ) );
		const T c33 = ( m00 * ( m11 * m22 - m12 * m21 ) - m01 * ( m10 * m22 - m12 * m20 ) + m02 * ( m10 * m21 - m11 * m20 ) );

		// Adjugate is the transpose of the cofactor matrix
		return Mat4<T>
		{
			Vec4<T>{ c00* invDet, c10* invDet, c20* invDet, c30* invDet },
				Vec4<T>{ c01* invDet, c11* invDet, c21* invDet, c31* invDet },
				Vec4<T>{ c02* invDet, c12* invDet, c22* invDet, c32* invDet },
				Vec4<T>{ c03* invDet, c13* invDet, c23* invDet, c33* invDet },
		};
	}

	// Fast inverse for pure rigid-body matrices (rotation + translation only, no scale/shear)
	// Considerably cheaper than the full cofactor inverse.
	template <typename T>
	[[nodiscard]] constexpr Mat4<T> inverseRigidbody(const Mat4<T>& m) noexcept
	{
		// Upper-left 3x3 is orthonormal -> its inverse is its transpose.
		// New translation = (R^T * t)
		const Vec3<T> t = { m(0, 3), m(1, 3), m(2, 3) };
		return Mat4<T>
		{
			Vec4<T>{ m(0, 0), m(0, 1), m(0, 2), -( m(0, 0) * t.x + m(0, 1) * t.y + m(0, 2) * t.z ) },
				Vec4<T>{ m(1, 0), m(1, 1), m(1, 2), -( m(1, 0) * t.x + m(1, 1) * t.y + m(1, 2) * t.z ) },
				Vec4<T>{ m(2, 0), m(2, 1), m(2, 2), -( m(2, 0) * t.x + m(2, 1) * t.y + m(2, 2) * t.z ) },
				Vec4<T>{ T(0), T(0), T(0), T(1) },
		};
	}

	// Transform factories
	template <typename T>
	[[nodiscard]] constexpr Mat4<T> makeTranslation(const Vec3<T>& t) noexcept
	{
		Mat4<T> m;
		m(0, 3) = t.x;
		m(1, 3) = t.y;
		m(2, 3) = t.z;
		return m;
	}

	template <typename T>
	[[nodiscard]] constexpr Mat4<T> makeScale(const Vec3<T>& s) noexcept
	{
		Mat4<T> m;
		m(0, 3) = s.x;
		m(1, 3) = s.y;
		m(2, 3) = s.z;
		return m;
	}

	template <typename T>
	[[nodiscard]] constexpr Mat4<T> makeScale(T uniform) noexcept
	{
		return makeScale(Vec3<T> {uniform, uniform, uniform});
	}

	// Rotation about an arbitrary normalised axis, angle in radians.
	// Left-handed convention - positive angle = clockwise when viewed from +axis tip.
	template <typename T>
	[[nodiscard]] inline Mat4<T> makeRotationAxis(const Vec3<T>& axis, T angle) noexcept
	{
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		const T t = T(1) - c;
		const T x = axis.x, y = axis.y, z = axis.z;

		return Mat4<T>{
			Vec4<T>{ t*x*x+c, t*x*y+s*z, t*x*z-s*y, T(0) },
			Vec4<T>{ t*x*y-s*z, t*y*y+c, t*y*z+s*x, T(0) },
			Vec4<T>{ t*x*z+s*y, t*y*z-s*x, t*z*z+c, T(0) },
			Vec4<T>{ T(0), T(0), T(0), T(1) },
		};
	}

	template <typename T>
	[[nodiscard]] inline Mat4<T> makeRotationX(T a) noexcept
	{
		return makeRotationAxis(Vec3<T>::unitX(), a);
	}

	template <typename T>
	[[nodiscard]] inline Mat4<T> makeRotationY(T a) noexcept
	{
		return makeRotationAxis(Vec3<T>::unitY(), a);
	}

	template <typename T>
	[[nodiscard]] inline Mat4<T> makeRotationZ(T a) noexcept
	{
		return makeRotationAxis(Vec3<T>::unitZ(), a);
	}

	// View matrix

	// Left-handed lookAt. eye looks toward target; up defines the up direction.
	template <typename T>
	[[nodiscard]] inline Mat4<T> makeLookAtLH(const Vec3<T>& eye, const Vec3<T> target, const Vec3<T>& up) noexcept
	{
		const Vec3<T> f = normalise(target - eye); // +Z forward
		const Vec3<T> r = normalise(cross(up, f)); // right
		const Vec3<T> u = cross(f, r); // reorthogonalised up

		return Mat4<T>
		{
			Vec4<T> { r.x, r.y, r.z, -dot(r, eye) },
			Vec4<T> { u.x, u.y, u.z, -dot(u, eye) },
			Vec4<T> { f.x, f.y, f.z, -dot(f, eye) },
			Vec4<T> { T(0), T(0), T(0), T(1) },
		};
	}

	// Projection matrices
	// All produce depth in [0, 1] -> DirectX and Vulkan but not OpenGL's [-1, 1]

	/// @brief Left-handed perspective projection.
	/// @param fovY - vertical field of view in radians
	/// @param aspect - width / height
	/// @param zNear - near clip plane (>0)
	/// @param zFar - far clip plane (>zNear)
	/// @return Mat4
	template <typename T>
	[[nodiscard]] inline Mat4<T> makePerspectiveLH(T fovY, T aspect, T zNear, T zFar) noexcept
	{
		assert(aspect > T(0));
		assert(zFar > zNear);
		assert(zNear > T(0));

		const T tanHalfFov = std::tan(fovY / T(2));
		const T yScale = T(1) / tanHalfFov;
		const T xScale = yScale / aspect;
		const T zRange = zFar - zNear;

		Mat4<T> m(T(0));
		m(0, 0) = xScale;
		m(1, 1) = xScale;
		m(2, 2) = zFar / zRange; // Maps [zNear, zFar] -> [0, 1]
		m(2, 3) = T(1); // w_clip = z_view (LH: positive z goes into screen)
		m(3, 2) = -( zNear * zFar ) / zRange;
		return m;
	}

	// Off-centre orthographic (useful for shadow maps, UI, tiled rendering)
	template <typename T>
	[[nodiscard]] constexpr Mat4<T> makeOrthographicOffcentreLH(T left, T right, T bottom, T top, T zNear, T zFar) noexcept
	{
		const T rml = right - left;
		const T tmb = top - bottom;
		const T zRange = zFar - zNear;

		Mat4<T> m(T(0));
		m(0, 0) = T(2) / rml;
		m(1, 1) = T(2) / tmb;
		m(2, 2) = T(1) / zRange;
		m(0, 3) = -( right + left ) / rml;
		m(1, 3) = -( top + bottom ) / tmb;
		m(2, 3) = -zNear / zRange;
		m(3, 3) = T(1);
		return m;
	}

	// Type defs
	using Mat4f = Mat4<float>;
	using Mat4d = Mat4<double>;
}
