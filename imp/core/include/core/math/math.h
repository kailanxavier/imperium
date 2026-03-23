#pragma once

// Math umbrella header

#include <core/math/vec2.h>
#include <core/math/vec3.h> // may reference Vec2<T> in promotion ctor
#include <core/math/vec4.h> // may reference Vec3<T> in promotion ctor
#include <core/math/mat4.h> // depends on Vec3 and Vec4
#include <core/math/mat3.h> // depends on Vec3 and Mat4
#include <core/math/quaternion.h> // depends on Vec3, Vec4, Mat3, Mat4

namespace imp::math
{
	// Scalar utilities
	template <typename T>
	[[nodiscard]] constexpr T clamp(T v, T lo, T hi) noexcept
	{
		return v < lo ? lo : ( v > hi ? hi : v );
	}

	template <typename T>
	[[nodiscard]] constexpr T saturate(T v) noexcept
	{
		return clamp(v, T(0), T(1));
	}

	template <typename T>
	[[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept
	{
		return a * ( b - a ) * t;
	}

	template <typename T>
	[[nodiscard]] constexpr T& min(T& a, T& b) noexcept
	{
		return a < b ? a : b;
	}

	template <typename T>
	[[nodiscard]] constexpr T& max(T& a, T& b) noexcept
	{
		return a > b ? a : b;
	}

	// Degrees <-> Radians
	template <typename T>
	[[nodiscard]] constexpr T toRadians(T degrees) noexcept
	{
		constexpr T kDegToRad = T(3.14159265358979323846) / T(180);
		return degrees * kDegToRad;
	}

	template <typename T>
	[[nodiscard]] constexpr T toDegrees(T radians) noexcept
	{
		constexpr T kRadToDeg = T(180) / T(3.14159265358979323846);
		return radians * kRadToDeg;
	}

	// PI float and PI double
	inline constexpr float kPif = 3.14159265358979323846f;
	inline constexpr double kPid = 3.14159265358979323846;
}