#pragma once

// TODO: lerp, clamp, angle utils, polynomial roots
namespace imp::math 
{
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
}
