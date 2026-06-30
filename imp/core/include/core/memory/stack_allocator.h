#pragma once

#include <core/memory/iallocator.h>

#include <cstring>
#include <cassert>

namespace imp::memory
{
	class StackAllocator final : public IAllocator
	{
	public:
		struct alignas( kMinAlignment ) Header
		{
			uint32_t prevOffset;
			uint32_t userBytes;
			MemTag tag;
			uint8_t padding[3];
		};

		static_assert( sizeof(Header) % kMinAlignment == 0, 
			"Header size must be a multiple of kMinAlignment" );

		StackAllocator(
			void* buffer,
			size_t bufferSize,
			std::string_view name = "StackAllocator") noexcept
			: IAllocator(name)
			, m_buffer(static_cast<std::byte*>( buffer ))
			, m_capacity(bufferSize)
			, m_cursor(0)
		{
			assert(buffer && "StackAllocator: null buffer");
			assert(bufferSize && "StackAllocator: zero-size buffer");
		}

		[[nodiscard]] void* alloc(
			size_t bytes,
			size_t alignment = kMinAlignment,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (bytes == 0)
				return nullptr;

			alignment = alignment < kMinAlignment ? kMinAlignment : alignment;

			const size_t headerStart = m_cursor;
			const size_t userStart = alignUp(headerStart + sizeof(Header), alignment);
			const size_t end = userStart + bytes;

			if (end > m_capacity)
			{
				assert(false && "StackAllocator: out of memory");
				return nullptr;
			}

			const size_t headerPos = userStart - sizeof(Header);
			Header* hdr = reinterpret_cast<Header*>( m_buffer + headerPos );
			hdr->prevOffset = static_cast<uint32_t>( headerStart );
			hdr->userBytes = static_cast<uint32_t>( bytes );
			hdr->tag = tag;

			m_cursor = end;
			m_stats.recordAlloc(bytes, tag);
			return m_buffer + userStart;
		}

		void free(
			void* ptr,
			size_t bytes,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (!ptr)
				return;

			const std::byte* userPtr = static_cast<const std::byte*>( ptr );
			assert(userPtr > m_buffer && userPtr < m_buffer + m_capacity
				&& "StackAllocator::free: pointer not in range");

			// Recover the header written just before the user block
			const Header* hdr = reinterpret_cast<const Header*>(userPtr - sizeof(Header));

			// The end of this allocation must be the current cursor
			assert(static_cast<size_t>(userPtr - m_buffer) + hdr->userBytes == m_cursor
				&& "StackAllocator::free: out of order free detected");

			(void)bytes;
			(void)tag;

			m_stats.recordFree(hdr->userBytes, hdr->tag);
			m_cursor = hdr->prevOffset;
		}
		
		void reset() noexcept override
		{
			m_cursor = 0;
			m_stats.reset();
		}

		struct Marker { size_t cursor; };
		[[nodiscard]] Marker getMarker() const noexcept { return { m_cursor }; }

		// No destructors are called, so we should only use for POD data
		void rewindTo(Marker m) noexcept
		{
			assert(m.cursor <= m_cursor && "StackAllocator: marker is ahead of cursor");
			
			// Approximate stats rewind (can't recover per-tag breakdown exactly).
			const size_t rewound = m_cursor - m.cursor;
			if (rewound > 0)
				m_stats.recordFree(rewound, MemTag::Scratch);

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

	class ScopedStackMarker
	{
	public:
		explicit ScopedStackMarker(StackAllocator& stack) noexcept
			: m_stack(stack), m_marker(stack.getMarker()) { }

		~ScopedStackMarker() noexcept { m_stack.rewindTo(m_marker); }

		ScopedStackMarker(const ScopedStackMarker&) = delete;
		ScopedStackMarker& operator=(const ScopedStackMarker&) = delete;

	private:
		StackAllocator& m_stack;
		StackAllocator::Marker m_marker;
	};
}
