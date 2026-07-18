#pragma once

#include <cmath>
#include <cassert>
#include <type_traits>

namespace imp::math
{
	template <typename T>
	struct Vec2
	{
		static_assert( std::is_integral_v<T> || std::is_floating_point_v<T>, "Vec2<T> requires a number type" );

		T x, y;

		constexpr Vec2() noexcept : x(T(0)), y(T(0)) {}
		constexpr explicit Vec2(T scalar) noexcept : x(scalar), y(scalar) {}
		constexpr Vec2(T x, T y) noexcept : x(x), y(y) {}

		// Conversion between different types
		template <typename U>
		constexpr explicit Vec2(const Vec2<U>& other) noexcept
			: x(static_cast<T>( other.x )), y(static_cast<T>( other.y )) {}

		// Array accessor
		[[nodiscard]] constexpr T& operator[](int i) noexcept
		{
			assert(i >= 0 && i < 2);
			return ( &x )[i];
		}
		[[nodiscard]] constexpr const T& operator[](int i) const noexcept
		{
			assert(i >= 0 && i < 2);
			return ( &x )[i];
		}

		// Constants
		[[nodiscard]] static constexpr Vec2 zero() noexcept { return { T(0), T(0) }; }
		[[nodiscard]] static constexpr Vec2 one() noexcept { return { T(1), T(1) }; }
		[[nodiscard]] static constexpr Vec2 unitX() noexcept { return { T(1), T(0) }; }
		[[nodiscard]] static constexpr Vec2 unitY() noexcept { return { T(0), T(1) }; }

		// Member arithmetic
		constexpr Vec2& operator+=(const Vec2& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
		constexpr Vec2& operator-=(const Vec2& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }
		constexpr Vec2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
		constexpr Vec2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }
		constexpr Vec2& operator*=(const Vec2& rhs) noexcept { x *= rhs.x; y *= rhs.y; return *this; }

		constexpr Vec2 operator-() const noexcept { return { -x, -y }; }

		[[nodiscard]] constexpr bool operator==(const Vec2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
		[[nodiscard]] constexpr bool operator!=(const Vec2& rhs) const noexcept { return !( *this == rhs ); }
	};

	// Free arithmetic operators
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator+(Vec2<T> lhs, const Vec2<T>& rhs) noexcept { return lhs += rhs; }
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator-(Vec2<T> lhs, const Vec2<T>& rhs) noexcept { return lhs -= rhs; }
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator*(Vec2<T> v, T s) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator*(T s, Vec2<T> v) noexcept { return v *= s; }
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator/(Vec2<T> v, T s) noexcept { return v /= s; }
	template <typename T> [[nodiscard]] constexpr Vec2<T> operator*(Vec2<T> lhs, const Vec2<T>& rhs) noexcept { return lhs *= rhs; } // component wise

	// Geometric operations
	template <typename T>
	[[nodiscard]] constexpr T dot(const Vec2<T>& a, const Vec2<T>& b) noexcept
	{
		return a.x * b.x + a.y * b.y;
	}

	template <typename T>
	[[nodiscard]] constexpr T cross(const Vec2<T>& a, const Vec2<T>& b) noexcept
	{
		return a.x * b.y - a.y * b.x;
	}

	template <typename T>
	[[nodiscard]] inline T lengthSq(const Vec2<T>& v) noexcept
	{
		return dot(v, v);
	}

	template <typename T>
	[[nodiscard]] inline T length(const Vec2<T>& v) noexcept
	{
		return std::sqrt(lengthSq(v));
	}

	template <typename T>
	[[nodiscard]] inline Vec2<T> normalise(const Vec2<T>& v) noexcept
	{
		const T len = length(v);
		assert(len > T(0) && "Normalise called on zero-length Vector2");
		return v / len;
	}

	template <typename T>
	[[nodiscard]] inline T distance(const Vec2<T>& a, const Vec2<T>& b) noexcept
	{
		return length(b - a);
	}

	template <typename T>
	[[nodiscard]] inline T distanceSq(const Vec2<T>& a, const Vec2<T>& b) noexcept
	{
		return lengthSq(b - a);
	}

	// Returns angle between two vector 2s in radians
	template <typename T>
	[[nodiscard]] inline T angleBetween(const Vec2<T>& a, const Vec2<T>& b) noexcept
	{
		const T d = dot(normalise(a), normalise(b));
		// clamp to guard against floating-point drift beyond [-1, 1]
		return std::acos(d < T(-1) ? T(-1) : ( d > T(1) ? T(1) : d ));
	}

	// Interpolation
	template <typename T>
	[[nodiscard]] constexpr Vec2<T> lerp(const Vec2<T>& a, const Vec2<T>& b, T t) noexcept
	{
		return a + ( b - a ) * t;
	}

	// Type defs
	using Vec2f = Vec2<float>;
	using Vec2i = Vec2<int>;
	using Vec2u = Vec2<unsigned int>;
	using Vec2d = Vec2<double>;
}
