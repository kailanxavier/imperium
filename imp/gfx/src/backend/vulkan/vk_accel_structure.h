#pragma once
#include "vk_buffer.h"
#include <gfx/resources.h>
#include <vulkan/vulkan.h>
#include <core/types/int_types.h>

namespace imp::gfx::vulkan
{
	class VulkanBlas : public gfx::IBlas
	{
	public:
		VulkanBlas() = default;
		~VulkanBlas() override;

		VulkanBlas(const VulkanBlas&) = delete;
		VulkanBlas& operator=(const VulkanBlas&) = delete;
		VulkanBlas(const VulkanBlas&&) = delete;
		VulkanBlas& operator=(const VulkanBlas&&) = delete;

		[[nodiscard]] u64 deviceAddress() const override { return static_cast<u64>( m_address ); }
		[[nodiscard]] VkAccelerationStructureKHR handle() const { return m_handle; }

		VkDevice m_device = VK_NULL_HANDLE;
		VkAccelerationStructureKHR m_handle = VK_NULL_HANDLE;
		VulkanBuffer m_backingBuffer;
		VkDeviceAddress m_address = 0;

		// TODO: 03/09/2026 - 
		// We're unsure wether we can use allocation callbacks for this,
		// since technically the only thing we're allocating is the backing buffer,
		// which does not use allocation callbacks.
		VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
