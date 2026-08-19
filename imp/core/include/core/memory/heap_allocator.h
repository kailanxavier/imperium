#pragma once

#include <core/memory/iallocator.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <source_location>

#ifndef IMP_MEMORY_TRACK_CALLSITES
#ifdef NDEBUG
#define IMP_MEMORY_TRACK_CALLSITES 0
#else
#define IMP_MEMORY_TRACK_CALLSITES 1
#endif
#endif

#if IMP_MEMORY_TRACK_CALLSITES
#include <unordered_map>
#include <mutex>
#include <vector>
#include <algorithm>
#endif

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
			const auto snapshot = statsSnapshot();

			if (snapshot.currentUsed != 0)
			{
				LOG_ERROR("Allocator", "Allocator '{}' destroyed with {} live bytes", name(), snapshot.currentUsed);
				for (size_t i = 0; i < static_cast<size_t>(MemTag::Count); ++i)
				{
					if (snapshot.tagBytes[i] != 0)
					{
						LOG_ERROR("Allocator", "{}: {} bytes",
							toString(static_cast<MemTag>(i)), snapshot.tagBytes[i]);
					}
				}

#if IMP_MEMORY_TRACK_CALLSITES
				dumpLiveAllocations();
#endif
			}

			assert(snapshot.currentUsed == 0 && "HeapAllocator destroyed with live allocations");
		}

		[[nodiscard]] void* alloc(
			size_t bytes,
			size_t alignment = kMinAlignment,
			MemTag tag = MemTag::Untagged,
			const std::source_location& loc = std::source_location::current()) noexcept override
		{
			if (bytes == 0)
				return nullptr;

			alignment = alignment < kMinAlignment ? kMinAlignment : alignment;
			void* ptr = detail::alignedMalloc(bytes, alignment);

			if (ptr)
			{
				m_stats.recordAlloc(bytes, tag);

#if IMP_MEMORY_TRACK_CALLSITES
				std::lock_guard lock(m_trackingMutex);
				m_live[ptr] = { bytes, tag, loc.file_name(), loc.function_name(), loc.line() };
#endif
			}

			return ptr;
		}

		void free(
			void* ptr,
			size_t bytes,
			MemTag tag = MemTag::Untagged,
			const std::source_location& loc = std::source_location::current()) noexcept override
		{
			if (!ptr)
				return;

#if IMP_MEMORY_TRACK_CALLSITES
			{
				std::lock_guard lock(m_trackingMutex);
				auto it = m_live.find(ptr);
				if (it == m_live.end())
				{
					LOG_ERROR("Allocator", "'{}': free() of untracked/already-freed pointer at {}:{}",
					   name(), loc.file_name(), loc.line());
				}
				else
				{
					if (it->second.bytes != bytes || it->second.tag != tag)
					{
						LOG_ERROR("Allocator",
						   "'{}': free() mismatch at {}:{} == allocated {} bytes as {} at {}:{}, freed as {} bytes as {}",
						   name(), loc.file_name(), loc.line(),
						   it->second.bytes, toString(it->second.tag), it->second.file, it->second.line,
						   bytes, toString(tag));
					}
					m_live.erase(it);
				}
			}
#endif

			m_stats.recordFree(bytes, tag);
			detail::alignedFree(ptr);
		}

		void reset() noexcept override {}

		[[nodiscard]] size_t capacity() const noexcept override { return 0; }
		[[nodiscard]] size_t remaining() const noexcept override { return SIZE_MAX; }

#if IMP_MEMORY_TRACK_CALLSITES
       void dumpLiveAllocations(size_t maxEntries = 20) const
       {
          struct SiteKey
          {
             const char* file;
             uint32_t line;
             MemTag tag;
             bool operator==(const SiteKey& o) const { return file == o.file && line == o.line && tag == o.tag; }
          };
          struct SiteKeyHash
          {
             size_t operator()(const SiteKey& k) const
             {
                return std::hash<const void*>()( k.file ) ^ ( std::hash<uint32_t>()( k.line ) << 1 )
                   ^ ( std::hash<uint32_t>()( static_cast<uint32_t>( k.tag ) ) << 2 );
             }
          };
          struct SiteTotals { size_t bytes = 0; size_t count = 0; const char* function = nullptr; };

          std::unordered_map<SiteKey, SiteTotals, SiteKeyHash> aggregated;
          {
             std::lock_guard lock(m_trackingMutex);
             for (const auto& [ptr, rec] : m_live)
             {
                SiteKey key{ rec.file, rec.line, rec.tag };
                auto& totals = aggregated[key];
                totals.bytes += rec.bytes;
                totals.count += 1;
                totals.function = rec.function;
             }
          }

          std::vector<std::pair<SiteKey, SiteTotals>> sorted(aggregated.begin(), aggregated.end());
          std::sort(sorted.begin(), sorted.end(),
             [](const auto& a, const auto& b) { return a.second.bytes > b.second.bytes; });

          LOG_ERROR("Allocator", "'{}': {} distinct leaking call site(s), showing top {}",
             name(), sorted.size(), std::min(maxEntries, sorted.size()));

			for (size_t i = 0; i < std::min(maxEntries, sorted.size()); ++i)
			{
				// We're not using the logger here because I broke it yesterday.
				// I'll fix it eventually, but for now we're not using it.
				const auto& [key, totals] = sorted[i];
				std::fprintf(stderr, "  %zu bytes across %zu allocation(s) [%.*s] %s:%u in %s\n",
					totals.bytes, totals.count,
					static_cast<int>(toString(key.tag).size()), toString(key.tag).data(),
					key.file, key.line, totals.function);
			}
			std::fflush(stderr);
       }
#endif

    private:
#if IMP_MEMORY_TRACK_CALLSITES
       struct AllocationRecord
       {
          size_t bytes;
          MemTag tag;
          const char* file;
          const char* function;
          uint32_t line;
       };

       mutable std::mutex m_trackingMutex;
       std::unordered_map<void*, AllocationRecord> m_live;
#endif
	};
}