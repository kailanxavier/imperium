#include "vk_accel_structure.h"
#include "vk_accel_structure_functions.h"

namespace imp::gfx::vulkan
{
	VulkanBlas::~VulkanBlas()
	{
		if (m_handle != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
			vkDestroyAccelerationStructureKHR_(m_device, m_handle, m_allocationCallbacks);
	}
}
