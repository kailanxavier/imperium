#if defined(IMP_GFX_VULKAN)

#include "vk_allocator.h"

#include <core/memory/allocator_utils.h>

#include <algorithm>
#include <cstring>

namespace imp::gfx::vulkan
{
	namespace
	{
		struct AllocationHeader
		{
			void* rawPtr;
			size_t rawSize;
			size_t requestedSize;
		};

		void* allocate(imp::memory::IAllocator& allocator, size_t size, size_t alignment)
		{
			const size_t effectiveAlign = std::max(alignment, alignof( AllocationHeader ));
			const size_t rawSize = size + effectiveAlign + sizeof(AllocationHeader);

			void* raw = allocator.alloc(rawSize, imp::memory::kMinAlignment, imp::memory::MemTag::Renderer);

			if (!raw) return nullptr;

			const uintptr_t basis = reinterpret_cast<uintptr_t>( raw ) + sizeof(AllocationHeader);
			const uintptr_t alignedAdrr = ( basis + effectiveAlign - 1 ) & ~( effectiveAlign - 1 );
			void* userPtr = reinterpret_cast<void*>( alignedAdrr );

			auto* header = reinterpret_cast<AllocationHeader*>( alignedAdrr ) - 1;
			header->rawPtr = raw;
			header->rawSize = rawSize;
			header->requestedSize = size;

			return userPtr;
		}

		void freePtr(imp::memory::IAllocator& allocator, void* userPtr)
		{
			if (!userPtr) return;
			auto* header = reinterpret_cast<AllocationHeader*>( userPtr ) - 1;
			allocator.free(header->rawPtr, header->rawSize, imp::memory::MemTag::Renderer);
		}

		void* VKAPI_CALL vkAllocationFn(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			if (size == 0) return nullptr;
			auto& allocator = *static_cast<imp::memory::IAllocator*>( pUserData );
			return allocate(allocator, size, alignment);
		}

		void* VKAPI_CALL vkReallocationFn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope)
		{
			auto& allocator = *static_cast<imp::memory::IAllocator*>( pUserData );
			if (!pOriginal) return vkAllocationFn(pUserData, size, alignment, scope);

			if (size == 0)
			{
				freePtr(allocator, pOriginal);
				return nullptr;
			}

			void* newPtr = allocate(allocator, size, alignment);

			if (!newPtr) return nullptr;

			auto* oldHeader = reinterpret_cast<AllocationHeader*>( pOriginal ) - 1;
			const size_t copySize = std::min(oldHeader->requestedSize, size);
			std::memcpy(newPtr, pOriginal, copySize);

			freePtr(allocator, pOriginal);
			return newPtr;
		}

		void VKAPI_CALL vkFreeFn(void* pUserData, void* pMemory)
		{
			auto& allocator = *static_cast<imp::memory::IAllocator*>( pUserData );
			freePtr(allocator, pMemory);
		}
	}

	VkAllocationCallbacks makeVulkanAllocationCallbacks(imp::memory::IAllocator& allocator) noexcept
	{
		VkAllocationCallbacks callbacks{};
		callbacks.pUserData = &allocator;
		callbacks.pfnAllocation = &vkAllocationFn;
		callbacks.pfnReallocation = &vkReallocationFn;
		callbacks.pfnFree = &vkFreeFn;
		return callbacks;
	}
}

#endif
