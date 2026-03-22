#pragma once

#include <cmath>
#include <cassert>
#include <type_traits>

namespace imp::math
{
	template <typename T>
	struct Vec4
	{
		static_assert( std::is_floating_point_v<T> || std::is_integral_v<T>, "Vec4<T> requires a number type" );

		T x, y, z, w;

		constexpr Vec4() noexcept : x(T(0)), y(T(0)), z(T(0)), w(T(0)) {}
		constexpr explicit Vec4(T scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}
		constexpr Vec4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

		// Promotion from Vec3 - w defaults to 0 (direction) or 1 (point)
		template <typename U>
		constexpr explicit Vec4(const Vec3<U>& xyz, T w = T(0)) noexcept
			: x(static_cast<T>( xyz.x )), y(static_cast<T>( xyz.y )), z(static_cast<T>( xyz.z )), w(w) {}

		// Conversion between different types
		template <typename U>
		constexpr explicit Vec4(const Vec4<U>& other) noexcept
			: x(static_cast<T>( other.x )), y(static_cast<T>( other.y )), z(static_cast<T>( other.z )), w(static_cast<T>( other.w )) {}

		// Array accessor
		[[nodiscard]] constexpr T& operator[](int i) noexcept
		{
			assert(i >= 0 && i < 4);
			return ( &x )[i];
		}

		[[nodiscard]] constexpr const T& operator[](int i) const noexcept
		{
			assert(i >= 0 && i < 4);
			return ( &x )[i];
		}

		// Subset extractors
		[[nodiscard]] constexpr Vec3<T> xyz() const noexcept { return { x, y, z }; }

		// Constants
		[[nodiscard]] static constexpr Vec4 zero() noexcept { return { T(0), T(0), T(0), T(0) }; }
		[[nodiscard]] static constexpr Vec4 one() noexcept { return { T(1), T(1), T(1), T(1) }; }
		// Homogenous point at origin
		[[nodiscard]] static constexpr Vec4 originPoint() noexcept { return { T(0), T(0), T(0), T(1) }; }

		// Member arithmetic
		constexpr Vec4& operator+=(const Vec4& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
		constexpr Vec4& operator-=(const Vec4& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }
		constexpr Vec4& operator*=(T s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
		constexpr Vec4& operator/=(T s) noexcept { x /= s; y /= s; z /= s; w *= s; return *this; }
		constexpr Vec4& operator*=(const Vec3& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; return *this; }

		constexpr Vec4 operator-() const noexcept { return { -x, -y, -z, -w }; }

		// Comparison
		[[nodiscard]] constexpr bool operator==(const Vec4& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
		[[nodiscard]] constexpr bool operator!=(const Vec4& rhs) const noexcept { return !( *this == rhs ); }
	};

	template <typename T> [[nodiscard]] constexpr Vec4<T> operator+(Vec4<T> lhs, const Vec4<T>& rhs) noexcept { return lhs += rhs; }
	template <typename T> [[nodiscard]] constexpr Vec4<T> operator-(Vec4<T> lhs, const Vec4<T>& rhs) noexcept { return lhs -= rhs; }
	template <typename T> [[nodiscard]] constexpr Vec4<T> operator*(Vec4<T> v, T s) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec4<T> operator*(T s, Vec4<T> v) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec4<T> operator/(Vec4<T> v, T s) noexcept { return v /= s; }
	template <typename T> [[nodiscard]] constexpr Vec4<T> operator*(Vec4<T> lhs, const Vec4<T>& rhs) noexcept { return lhs *= rhs; } // component wise

	// Geometric operations
	template <typename T>
	[[nodiscard]] constexpr T dot(const Vec4<T>& a, const Vec4<T>& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	template <typename T>
	[[nodiscard]] inline T lengthSq(const Vec4<T>& v) noexcept
	{
		return dot(v, v);
	}

	template <typename T>
	[[nodiscard]] inline T length(const Vec4<T>& v) noexcept
	{
		return std::sqrt(lengthSq(v));
	}

	template <typename T>
	[[nodiscard]] inline Vec4<T> normalise(const Vec4<T>& v) noexcept
	{
		const T len = length(v);
		assert(len > T(0) && "Normalise called on zero-length Vector4");
		return v / len;
	}

	template <typename T>
	[[nodiscard]] inline T distance(const Vec4<T>& a, const Vec4<T>& b) noexcept
	{
		return length(b - a);
	}

	template <typename T>
	[[nodiscard]] inline T distanceSq(const Vec4<T>& a, const Vec4<T>& b) noexcept
	{
		return lengthSq(b - a);
	}

	// Returns angle between two vector 4s in radians
	template <typename T>
	[[nodiscard]] inline T angleBetween(const Vec4<T>& a, const Vec4<T>& b) noexcept
	{
		const T d = dot(normalise(a), normalise(b));
		// clamp to guard against floating-point drift beyond [-1, 1]
		return std::acos(d < T(-1) ? T(-1) : ( d > T(1) ? T(1) : d ));
	}

	// Interpolation
	template <typename T>
	[[nodiscard]] constexpr Vec4<T> lerp(const Vec4<T>& a, const Vec4<T>& b, T t) noexcept 
	{
		return a + (b - a) * t;
	}

	template <typename T>
	[[nodiscard]] inline Vec4<T> slerp(const Vec4<T>& a, const Vec4<T>& b, T t) noexcept 
	{
		constexpr T kEpsilon = T(1e-6);
		T cosAngle = dot(a, b);

		// Ensure shortest path
		Vec4<T> b2 = b;
		if (cosAngle < T(0)) { cosAngle = -cosAngle; b2 = -b; }

		const T clampedCos = cosAngle > T(1) ? T(1) : cosAngle;
		const T angle = std::acos(clampedCos);

		if (angle < kEpsilon) {
			return lerp(a, b2, t);
		}

		const T sinAngle = std::sin(angle);
		return a * ( std::sin(( T(1) - t ) * angle) / sinAngle )
			+ b2 * ( std::sin(t * angle) / sinAngle );
	}

	// Type defs
	using Vec4f = Vec4<float>;
	using Vec4d = Vec4<double>;
}
