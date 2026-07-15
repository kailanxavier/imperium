#pragma once

#include <core/memory/iallocator.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

namespace imp::memory
{
	namespace detail
	{
		[[nodiscard]] inline void* alignedMalloc(size_t bytes, size_t alignment) noexcept
		{
			assert(isPowerOfTwo(alignment));
			const size_t rounded = alignUp(bytes, alignment);
#if defined(_MSC_VER)
			return ::_aligned_malloc(rounded, alignment);
#else
			if (alignment < sizeof(void*))
				return ::malloc(rounded);

			void* ptr = nullptr;
			if (::posix_memalign(&ptr, alignment, rounded) != 0)
				return nullptr;

			return ptr;
#endif
		}

		inline void alignedFree(void* ptr) noexcept
		{
#if defined(_MSC_VER)
			::_aligned_free(ptr);
#else
			::free(ptr);
#endif
		}
	}

	class HeapAllocator final : public IAllocator
	{
	public:
		explicit HeapAllocator(std::string_view name = "HeapAllocator") noexcept : IAllocator(name) {}
		~HeapAllocator() override
		{
			const size_t n = m_stats.currentUsed.load(std::memory_order_relaxed);
			assert(n == 0 && "HeapAllocator destroyed with live allocations");
		}

		[[nodiscard]] void* alloc(
			size_t bytes,
			size_t alignment = kMinAlignment,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (bytes == 0)
				return nullptr;

			alignment = alignment < kMinAlignment ? kMinAlignment : alignment;
			void* ptr = detail::alignedMalloc(bytes, alignment);

			if (ptr)
				m_stats.recordAlloc(bytes, tag);

			return ptr;
		}

		void free(
			void* ptr,
			size_t bytes,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (!ptr)
				return;

			m_stats.recordFree(bytes, tag);
			detail::alignedFree(ptr);
		}

		void reset() noexcept override {}

		[[nodiscard]] size_t capacity() const noexcept override { return 0; }
		[[nodiscard]] size_t remaining() const noexcept override { return SIZE_MAX; }
	};
}