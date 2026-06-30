#pragma once

#include <core/memory/allocator_utils.h>
#include <core/memory/allocator_types.h>

#include <cstddef>
#include <string_view>
#include <new> // std::bad_alloc

namespace imp::memory
{
	class IAllocator
	{
	public:
		explicit IAllocator(std::string_view name) noexcept : m_name(name) {}
		virtual ~IAllocator() = default;

		IAllocator(const IAllocator&) = delete;
		IAllocator& operator=(const IAllocator&) = delete;

		// Allocate bytes with at least alignment-byte alignment.
		[[nodiscard]] virtual void* alloc(
			size_t bytes, 
			size_t alignment = kMinAlignment, 
			MemTag tag = MemTag::Untagged) noexcept = 0;

		[[nodiscard]] virtual void free(
			void* ptr, 
			size_t bytes,
			MemTag tag = MemTag::Untagged) noexcept = 0;

		virtual void reset() noexcept {}

		[[nodiscard]] std::string_view name() const noexcept { return m_name; }
		[[nodiscard]] const AllocatorStats& stats() const noexcept { return m_stats; }
		[[nodiscard]] AllocatorStatsSnapshot statsSnapshot() const noexcept { return snapshot(m_stats); }

		// Capacity in bytes, if the allocator is backed by a fixed buffer.
		[[nodiscard]] virtual size_t capacity() const noexcept { return 0; }

		// Bytes currently available before the allocator is exhausted.
		[[nodiscard]] virtual size_t remaining() const noexcept { return SIZE_MAX; }

	protected:
		AllocatorStats m_stats;

	private:
		std::string_view m_name;
	};

	// Allocate and construct a T with forwarded constructor arguments.
	template <typename T, typename... Args>
	[[nodiscard]] T* allocNew(IAllocator& a, MemTag tag, Args&&... args)
	{
		void* mem = a.alloc(sizeof(T), alignof( T ), tag);
		if (!mem)
			return nullptr;

		return ::new( mem ) T(static_cast<Args&&>( args )...);
	}

	// Destroy and free a T previously created with allocNew().
	template <typename T>
	void allocDelete(IAllocator& a, T* ptr, MemTag tag) noexcept
	{
		if (!ptr)
			return;

		ptr->~T();
		a.free(ptr, sizeof(T), tag);
	}
}