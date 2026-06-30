#pragma once

#include <core/memory/iallocator.h>

#include <cstring>
#include <cassert>

namespace imp::memory
{
	class LinearAllocator final : public IAllocator
	{
	public:
		LinearAllocator(
			void* buffer,
			size_t bufferSize,
			std::string_view name = "LinearAllocator") noexcept 
			: IAllocator(name)
			, m_buffer(static_cast<std::byte*>(buffer ))
			, m_capacity(bufferSize)
			, m_cursor(0)
		{ 
			assert(buffer && "LinearAllocator: null buffer");
			assert(bufferSize && "LinearAllocator: zero-size buffer");
		}

		[[nodiscard]] void* alloc(
			size_t bytes,
			size_t alignment = kMinAlignment,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (bytes == 0)
				return nullptr;

			alignment = alignment < kMinAlignment ? kMinAlignment : alignment;

			const size_t pad = alignmentPadding(m_buffer + m_cursor, alignment);
			const size_t needed = pad + bytes;

			if (m_cursor + needed > m_capacity)
			{
				assert(false && "LinearAllocator: out of memory");
				return nullptr;
			}

			void* ptr = m_buffer + m_cursor + pad;
			m_cursor += needed;

			m_stats.recordAlloc(bytes, tag);
			return ptr;
		}

		void free(
			void* ptr,
			size_t bytes,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (!ptr || bytes == 0)
				return;

			m_stats.recordFree(bytes, tag);
		}

		void reset() noexcept override
		{
			m_cursor = 0;
			m_stats.reset();
		}

		struct Marker { size_t cursor; };
		[[nodiscard]] Marker getMarker() const noexcept { return { m_cursor }; }

		void rewindTo(Marker m) noexcept
		{
			assert(m.cursor <= m_cursor && "LinearAllocator: marker is ahead of cursor");

			// Adjust stats: notional-free the bytes we're rewinding over.
			const size_t rewound = m_cursor - m.cursor;
			if (rewound > 0)
			{
				// We can't recover the original tag breakdown, so use Scratch.
				m_stats.recordFree(rewound, MemTag::Scratch);
			}

			m_cursor = m.cursor;
		}

		[[nodiscard]] size_t capacity() const noexcept override { return m_capacity; }
		[[nodiscard]] size_t remaining() const noexcept override { return m_capacity - m_cursor; }
		[[nodiscard]] size_t cursor() const noexcept { return m_cursor; }

	private:
		std::byte* m_buffer;
		size_t m_capacity;
		size_t m_cursor;
	};

	// ScopedLinearReset
	// RAII guard that resets a LinearAllocator when it goes out of scope.

	class ScopedLinearReset
	{
	public:
		explicit ScopedLinearReset(LinearAllocator& arena) noexcept
			: m_arena(arena), m_marker(arena.getMarker()) { }

		~ScopedLinearReset() noexcept { m_arena.rewindTo(m_marker); }

		ScopedLinearReset(const ScopedLinearReset&) = delete;
		ScopedLinearReset& operator=(const ScopedLinearReset&) = delete;

	private:
		LinearAllocator& m_arena;
		LinearAllocator::Marker m_marker;
	};
}
