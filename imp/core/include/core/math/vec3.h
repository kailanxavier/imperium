#pragma once

#include <cmath>
#include <cassert>
#include <type_traits>
#include <core/math/vec2.h>

namespace imp::math
{
	template <typename T>
	struct Vec3
	{
		static_assert( std::is_floating_point_v<T> || std::is_integral_v<T>, "Vec3<T> requires a number type" );

		T x, y, z;

		constexpr Vec3() noexcept : x(T(0)), y(T(0)), z(T(0)) {}
		constexpr explicit Vec3(T scalar) noexcept : x(scalar), y(scalar), z(scalar) {}
		constexpr Vec3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}
		constexpr explicit Vec3(Vec2<T> v) noexcept : x(v.x), y(v.y), z(T(0)) {}
		constexpr explicit Vec3(Vec2<T> v, T z) noexcept : x(v.x), y(v.y), z(z) {}

		// Conversion between different types
		template <typename U>
		constexpr explicit Vec3(const Vec3<U>& other) noexcept 
			: x(static_cast<T>( other.x )), y(static_cast<T>( other.y )), z(static_cast<T>( other.z )) {}

		// Array accessors
		[[nodiscard]] constexpr T& operator[](int i) noexcept
		{
			assert(i >= 0 && i < 3);
			return ( &x )[i];
		}

		[[nodiscard]] constexpr const T& operator[](int i) const noexcept
		{
			assert(i >= 0 && i < 3);
			return ( &x )[i];
		}

		// Constants
		[[nodiscard]] static constexpr Vec3 zero() noexcept { return { T(0), T(0), T(0) }; }
		[[nodiscard]] static constexpr Vec3 one() noexcept { return { T(1), T(1), T(1) }; }
		[[nodiscard]] static constexpr Vec3 unitX() noexcept { return { T(1), T(0), T(0) }; }
		[[nodiscard]] static constexpr Vec3 unitY() noexcept { return { T(0), T(1), T(0) }; }
		[[nodiscard]] static constexpr Vec3 unitZ() noexcept { return { T(0), T(0), T(1) }; }
		[[nodiscard]] static constexpr Vec3 up() noexcept { return unitY(); }
		[[nodiscard]] static constexpr Vec3 right() noexcept { return unitX(); }
		[[nodiscard]] static constexpr Vec3 forward() noexcept { return -unitZ(); }

		// Member arithmetic
		constexpr Vec3& operator+=(const Vec3& rhs) noexcept { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
		constexpr Vec3& operator-=(const Vec3& rhs) noexcept { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
		constexpr Vec3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
		constexpr Vec3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }
		constexpr Vec3& operator*=(const Vec3& rhs) noexcept { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }

		constexpr Vec3 operator-() const noexcept { return { -x, -y, -z }; }

		[[nodiscard]] constexpr bool operator==(const Vec3& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }
		[[nodiscard]] constexpr bool operator!=(const Vec3& rhs) const noexcept { return !( *this == rhs ); }
	};

	// Free arithmetic operators
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator+(Vec3<T> lhs, const Vec3<T>& rhs) noexcept { return lhs += rhs; }
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator-(Vec3<T> lhs, const Vec3<T>& rhs) noexcept { return lhs -= rhs; }
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator*(Vec3<T> v, T s) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator*(T s, Vec3<T> v) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator/(Vec3<T> v, T s) noexcept { return v /= s; }
	template <typename T> [[nodiscard]] constexpr Vec3<T> operator*(Vec3<T> lhs, const Vec3<T>& rhs) noexcept { return lhs *= rhs; } // component wise

	// Geometric operations
	template <typename T>
	[[nodiscard]] constexpr T dot(const Vec3<T>& a, const Vec3<T>& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	template <typename T>
	[[nodiscard]] constexpr Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b) noexcept
	{
		return {
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		};
	}

	template <typename T>
	[[nodiscard]] inline T lengthSq(const Vec3<T> v) noexcept
	{
		return dot(v, v);
	}

	template <typename T>
	[[nodiscard]] inline T length(const Vec3<T> v) noexcept
	{
		return std::sqrt(lengthSq(v));
	}

	template <typename T>
	[[nodiscard]] inline Vec3<T> normalise(const Vec3<T>& v) noexcept
	{
		const T len = length(v);
		assert(len > T(0) && "Normalise called on zero-length Vector3");
		return v / len;
	}

	template <typename T>
	[[nodiscard]] inline T distance(const Vec3<T>& a, const Vec3<T>& b) noexcept
	{
		return length(b - a);
	}

	template <typename T>
	[[nodiscard]] inline T distanceSq(const Vec3<T>& a, const Vec3<T>& b) noexcept
	{
		return lengthSq(b - a);
	}

	// Returns angle between two vector 3s in radians
	template <typename T>
	[[nodiscard]] inline T angleBetween(const Vec3<T>& a, const Vec3<T>& b) noexcept
	{
		const T d = dot(normalise(a), normalise(b));
		// clamp to guard against floating-point drift beyond [-1, 1]
		return std::acos(d < T(-1) ? T(-1) : ( d > T(1) ? T(1) : d ));
	}

	// Reflect v about normal n (n must be normalised)
	template <typename T>
	[[nodiscard]] constexpr Vec3<T> reflect(const Vec3<T>& v, const Vec3<T>& n) noexcept
	{
		return v - n * ( T(2) * dot(v, n) );
	}

	// Interpolation
	template <typename T>
	[[nodiscard]] constexpr Vec3<T> lerp(const Vec3<T>& a, const Vec3<T>& b, T t) noexcept
	{
		return a + ( b - a ) * t;
	}

	template <typename T>
	[[nodiscard]] inline Vec3<T> slerp(const Vec3<T>& a, const Vec3<T>& b, T t) noexcept
	{
		constexpr T kEpsilon = T(1e-6);
		const T cosAngle = dot(a, b);
		const T clampedCos = cosAngle < T(-1) ? T(-1) : ( cosAngle > T(1) ? T(1) : cosAngle );
		const T angle = std::acos(clampedCos);

		if (angle < kEpsilon)
			return lerp(a, b, t);

		const T sinAngle = std::sin(angle);
		return a * ( std::sin(( T(1) - t ) * angle) / sinAngle ) 
			+ b * ( std::sin(t * angle) / sinAngle );
	}

	// Type defs
	using Vec3f = Vec3<float>;
	using Vec3i = Vec3<int>;
	using Vec3u = Vec3<unsigned int>;
	using Vec3d = Vec3<double>;
}
