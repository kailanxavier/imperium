#pragma once

#include <vulkan/vulkan.h>
#include <core/types/int_types.h>

namespace imp::gfx::vulkan
{
	bool loadAccelStructFunctions(VkDevice device);

	VkResult vkCreateAccelerationStructureKHR_(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure);

	void vkDestroyAccelerationStructureKHR_(VkDevice device, VkAccelerationStructureKHR accelerationStructure,
		const VkAllocationCallbacks* pAllocator);

	void vkGetAccelerationStructureBuildSizesKHR_(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType,
		const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const u32* pMaxPrimitiveCounts,
		VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo);

	void vkCmdBuildAccelerationStructuresKHR_(VkCommandBuffer commandBuffer, u32 infoCount,
		const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
		const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);

	VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR_(VkDevice device,
		const VkAccelerationStructureDeviceAddressInfoKHR* pInfo);
}
