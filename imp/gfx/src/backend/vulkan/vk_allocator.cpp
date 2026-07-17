#include "vk_allocator.h"

#include <core/memory/allocator_utils.h>
#include <core/memory/int_types.h>

#include <algorithm>

namespace imp::gfx::vulkan
{
	namespace
	{
		struct AllocationHeader
		{
			void* rawPtr;
			size_t rawSize;
			size_t requestedSize;
			u64 magic;
		};

		void* allocate(memory::IAllocator& allocator, size_t size, size_t alignment)
		{
			const size_t effectiveAlign = std::max(alignment, alignof( AllocationHeader ));
			const size_t rawSize = size + effectiveAlign + sizeof(AllocationHeader);

			void* raw = allocator.alloc(rawSize, std::max(memory::kMinAlignment, effectiveAlign), memory::MemTag::Renderer);

			if (!raw) return nullptr;

			uintptr_t start = reinterpret_cast<uintptr_t>(raw) + sizeof(AllocationHeader);
			uintptr_t aligned = memory::alignUp(start, effectiveAlign);
			void* userPtr = reinterpret_cast<void*>(aligned);

			auto* header = reinterpret_cast<AllocationHeader*>( aligned - sizeof(AllocationHeader) );
			header->rawPtr = raw;
			header->rawSize = rawSize;
			header->requestedSize = size;
			header->magic = 0xDEADBEEFCAFEBABE;

			return userPtr;
		}

		void freePtr(memory::IAllocator& allocator, void* userPtr)
		{
			if (!userPtr)
				return;

			auto* header =
				reinterpret_cast<AllocationHeader*>(
					reinterpret_cast<uintptr_t>(userPtr) - sizeof(AllocationHeader)
				);

			assert(header->magic == 0xDEADBEEFCAFEBABE && "Magic was garbage");

			allocator.free(
				header->rawPtr,
				header->rawSize,
				memory::MemTag::Renderer
			);
		}

		void* VKAPI_CALL vkAllocationFn(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope)
		{
			if (size == 0) return nullptr;
			auto& allocator = *static_cast<memory::IAllocator*>( pUserData );
			return allocate(allocator, size, alignment);
		}

		void* VKAPI_CALL vkReallocationFn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope)
		{
			auto& allocator = *static_cast<memory::IAllocator*>( pUserData );
			if (!pOriginal) return vkAllocationFn(pUserData, size, alignment, scope);

			if (size == 0)
			{
				freePtr(allocator, pOriginal);
				return nullptr;
			}

			void* newPtr = allocate(allocator, size, alignment);

			if (!newPtr) return nullptr;

			auto* oldHeader =
				reinterpret_cast<AllocationHeader*>(
					reinterpret_cast<uintptr_t>(pOriginal) - sizeof(AllocationHeader)
				);

			assert(oldHeader->magic == 0xDEADBEEFCAFEBABE && "Magic was garbage");

			const size_t copySize = std::min(oldHeader->requestedSize, size);
			std::memcpy(newPtr, pOriginal, copySize);

			freePtr(allocator, pOriginal);
			return newPtr;
		}

		void VKAPI_CALL vkFreeFn(void* pUserData, void* pMemory)
		{
			auto& allocator = *static_cast<memory::IAllocator*>( pUserData );
			freePtr(allocator, pMemory);
		}
	}

	VkAllocationCallbacks makeVulkanAllocationCallbacks(memory::IAllocator& allocator) noexcept
	{
		VkAllocationCallbacks callbacks{};
		callbacks.pUserData = &allocator;
		callbacks.pfnAllocation = &vkAllocationFn;
		callbacks.pfnReallocation = &vkReallocationFn;
		callbacks.pfnFree = &vkFreeFn;
		return callbacks;
	}
}

