#pragma once
#include <vulkan/vulkan.h>

#include <core/memory/iallocator.h>

namespace imp::gfx::vulkan
{
	[[nodiscard]] VkAllocationCallbacks makeVulkanAllocationCallbacks(imp::memory::IAllocator& allocator) noexcept;
}
