#pragma once

#include "iallocator.h"

#include <cassert>
#include <cstring>
#include <type_traits>

namespace imp::memory
{
	class PoolAllocator final : public IAllocator
	{
	public:
		PoolAllocator(
			void* buffer,
			size_t bufferSize,
			size_t blockSize,
			size_t blockAlignment = kMinAlignment,
			std::string_view name = "PoolAllocator") noexcept
			: IAllocator(name)
			, m_buffer(static_cast<std::byte*>(buffer ))
			, m_blockSize(0)
			, m_blockCount(0)
			, m_freeHead(nullptr) 
		{
			assert(buffer && "PoolAllocator: null buffer");
			assert(bufferSize && "PoolAllocator: zero-size buffer");
			assert(blockSize && "PoolAllocator: zero block size");
			assert(isPowerOfTwo(blockAlignment));

			// Each block must be large enough to hold the intrusive next pointer
			// AND satisfy the requested alignment
			const size_t minBlock = sizeof(void*);
			blockSize = blockSize < minBlock ? minBlock : blockSize;
			blockAlignment = blockAlignment < kMinAlignment ? kMinAlignment : blockAlignment;

			m_blockSize = alignUp(blockSize, blockAlignment);

			// Align the buffer start, then carve out as many blocks as fit.
			void* aligned = alignPtr(buffer, blockAlignment);
			size_t overhead = static_cast<size_t>(
				static_cast<std::byte*>(aligned) - static_cast<std::byte*>(buffer));

			if (overhead >= bufferSize)
			{
				assert(false && "PoolAllocator: buffer too small after alignment");
				return;
			}

			m_buffer = static_cast<std::byte*>( aligned );
			m_blockCount = ( bufferSize - overhead ) / m_blockSize;

			assert(m_blockCount > 0 && "PoolAllocator: buffer too small for even one block");

			buildFreeList();
		}

		[[nodiscard]] void* alloc(
			size_t /*bytes*/,
			size_t /*alignment*/ = kMinAlignment,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (!m_freeHead)
			{
				assert(false && "PoolAllocator: pool exhausted");
				return nullptr;
			}

			void* ptr = m_freeHead;
			m_freeHead = *reinterpret_cast<void**>( m_freeHead ); // pop

			m_stats.recordAlloc(m_blockSize, tag);
			return ptr;
		}

		void free(
			void* ptr,
			size_t /*bytes*/,
			MemTag tag = MemTag::Untagged) noexcept override
		{
			if (!ptr)
				return;

			assertInRange(ptr);

			// Write old head into the block (essentially pushing onto the list)
			*reinterpret_cast<void**>( ptr ) = m_freeHead;
			m_freeHead = ptr;

			m_stats.recordFree(m_blockSize, tag);
		}
		
		void reset() noexcept override
		{
			buildFreeList();
			m_stats.reset();
		}

		[[nodiscard]] size_t blockSize() const noexcept { return m_blockSize; }
		[[nodiscard]] size_t blockCount() const noexcept { return m_blockCount; }
		[[nodiscard]] size_t capacity() const noexcept override { return m_blockSize * m_blockCount; }
		[[nodiscard]] size_t remaining() const noexcept override
		{
			size_t n = 0;
			void* p = m_freeHead;
			while (p) 
			{
				++n;
				p = *reinterpret_cast<void**>( p );
			}
			return n * m_blockSize;
		}

	private:
		void buildFreeList() noexcept
		{
			m_freeHead = nullptr;

			// We want to thread blocks into a free list in reverse order
			// so the first alloc() return block 0 (cache-friendly access pattern)
			for (size_t i = m_blockCount; i-- > 0;)
			{
				void* block = m_buffer + i * m_blockSize;
				*reinterpret_cast<void**>( block ) = m_freeHead; // c++ is cool as fuck
				m_freeHead = block;
			}
		}

		void assertInRange([[maybe_unused]] const void* ptr) const noexcept
		{
			[[maybe_unused]] const std::byte* p = static_cast<const std::byte*>( ptr );
			[[maybe_unused]] const std::byte* end = m_buffer + m_blockCount * m_blockSize;
			assert(p >= m_buffer && p < end && "PoolAllocator::free: pointer is not in pool");
			assert(( p - m_buffer ) % m_blockSize == 0
				&& "PoolAllocator::free: pointer not on a block boundary");
		}

		std::byte* m_buffer;
		size_t m_blockSize;
		size_t m_blockCount;
		void* m_freeHead;
	};

	template <typename T>
	class TypedPool
	{
	public:
		TypedPool(void* buffer, size_t bufferSize, std::string_view name = "TypedPool") noexcept
			: m_pool(buffer, bufferSize, sizeof(T), alignof( T ), name)
		{ }

		template <typename... Args>
		[[nodiscard]] T* create(MemTag tag, Args&&... args) noexcept
		{
			void* mem = m_pool.alloc(sizeof(T), alignof( T ), tag);

			if (!mem)
				return nullptr;

			return ::new( mem ) T(static_cast<Args&&>( args )...);
		}

		void destroy(T* ptr, MemTag tag) noexcept
		{
			if (!ptr)
				return;

			ptr->~T();
			m_pool.free(ptr, sizeof(T), tag);
		}

		[[nodiscard]] PoolAllocator& pool() noexcept { return m_pool; }
		[[nodiscard]] const PoolAllocator& pool() const noexcept { return m_pool; }

	private:
		PoolAllocator m_pool;
	};
}