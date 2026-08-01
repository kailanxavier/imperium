#pragma once
#include <core/types/int_types.h>
#include <functional>

namespace imp::core
{
	template <typename Tag>
	struct Handle
	{
		static constexpr u32 kInvalidIndex = 0xFFFFFFFFU;

		u32 index = kInvalidIndex;
		u32 generation = 0;

		[[nodiscard]] bool isValid() const noexcept { return index != kInvalidIndex; }
	};

	template <typename Tag>
	inline bool operator==(const Handle<Tag>& a, const Handle<Tag>& b) noexcept
	{
		return a.index == b.index && a.generation == b.generation;
	}

	template <typename Tag>
	inline bool operator!=(const Handle<Tag>& a, const Handle<Tag>& b) noexcept
	{
		return !( a == b );
	}

	template <typename Tag>
	inline bool operator<(const Handle<Tag>& a, const Handle<Tag>& b) noexcept
	{
		if (a.index != b.index) return a.index < b.index;
		return a.generation < b.generation;
	}
}

namespace std
{
	template<typename Tag>
	struct hash<imp::core::Handle<Tag>>
	{
		size_t operator()(const imp::core::Handle<Tag>& h) const noexcept
		{
			return ( static_cast<size_t>( h.index ) << 32 ) ^ static_cast<size_t>( h.generation );
		}
	};
}
