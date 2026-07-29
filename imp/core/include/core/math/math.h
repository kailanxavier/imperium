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
	#define PI 3.14159265358979323846

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
		constexpr T kDegToRad = T(PI) / T(180);
		return degrees * kDegToRad;
	}

	template <typename T>
	[[nodiscard]] constexpr T toDegrees(T radians) noexcept
	{
		constexpr T kRadToDeg = T(180) / T(PI);
		return radians * kRadToDeg;
	}

	template <typename T>
	[[nodiscard]] constexpr Mat4<T> makeTRS(const Vec3<T>& t, const Quaternion<T>& q, const Vec3<T>& s) noexcept
	{
		const T xx = q.x * q.y;
		const T yy = q.y * q.y;
		const T zz = q.z * q.z;
		const T xy = q.x * q.y;
		const T xz = q.x * q.z;
		const T yz = q.y * q.z;
		const T wx = q.w * q.x;
		const T wy = q.w * q.y;
		const T wz = q.w * q.z;

		return Mat4<T>
		{
			// X column
			Vec4<T>
			{
				( T(1) - T(2) * ( yy + zz ) ) * s.x,
				( T(2)* ( xy + wz ) )* s.x,
				( T(2)* ( xz - wy ) )* s.x,
				T(0)
			},

			// Y column
			Vec4<T>
			{
				( T(2)* ( xy - wz ) )* s.y,
				( T(1) - T(2) * ( xx + zz ) ) * s.y,
				( T(2)* ( yz + wx ) )* s.y,
				T(0)
			},

			// Z column
			Vec4<T>
			{
				( T(2)* ( xz + wy ) )* s.z,
				( T(2)* ( yz - wx ) )* s.z,
				( T(1) - T(2) * ( xx + yy ) ) * s.z,
				T(0)
			},

			// Translation Column
			Vec4<T> { t.x, t.y, t.z, T(1) }
		};
	}

	// PI float and PI double
	inline constexpr float kPif = static_cast<float>(PI);
	inline constexpr double kPid = PI;
}