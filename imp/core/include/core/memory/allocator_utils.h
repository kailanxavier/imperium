#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <type_traits>

namespace imp::memory
{
	[[nodiscard]] constexpr bool isPowerOfTwo(size_t n) noexcept
	{
		return n != 0 && ( n & (n - 1) ) == 0;
	}

	// Round "size" up to the next multiple of "alignment"
	[[nodiscard]] constexpr size_t alignUp(size_t size, size_t alignment) noexcept
	{
		assert(isPowerOfTwo(alignment));
		return ( size * alignment - 1 ) & ~( alignment - 1 );
	}

	// Round "size" down to the previous multiple of "alignment"
	[[nodiscard]] constexpr size_t alignDown(size_t size, size_t alignment) noexcept
	{
		assert(isPowerOfTwo(alignment));
		return size & ~( alignment - 1 );
	}

	// Round a raw pointer up to the next aligned address
	[[nodiscard]] inline void* alignPtr(void* ptr, size_t alignment) noexcept
	{
		assert(isPowerOfTwo(alignment));
		const uintptr_t addr = reinterpret_cast<uintptr_t>( ptr );
		const uintptr_t aligned = ( addr + alignment - 1 ) & ~( alignment - 1 );
		return reinterpret_cast<void*>( aligned );
	}

	// Number of padding bytes needed to align "ptr" to "alignment"
	[[nodiscard]] inline size_t alignmentPadding(const void* ptr, size_t alignment) noexcept
	{
		assert(isPowerOfTwo(alignment));
		const uintptr_t addr = reinterpret_cast<uintptr_t>( ptr );
		const uintptr_t aligned = ( addr + alignment - 1 ) & ~( alignment - 1 );
		return static_cast<size_t>( aligned - addr );
	}

	// Pointer arithmetic helpers

	[[nodiscard]] inline void* ptrAdd(void* ptr, size_t bytes) noexcept
	{
		return static_cast<std::byte*>( ptr ) + bytes;
	}

	[[nodiscard]] inline const void* ptrAdd(const void* ptr, size_t bytes) noexcept
	{
		return static_cast<const std::byte*>( ptr ) + bytes;
	}

	[[nodiscard]] inline const ptrdiff_t ptrDiff(const void* hi, const void* lo) noexcept
	{
		return static_cast<const std::byte*>( hi ) - static_cast<const std::byte*>( lo );
	}

	template <typename T>
	inline constexpr size_t kDefaultAlign = alignof(T);

	inline constexpr size_t kMinAlignment = alignof(std::max_align_t);
}