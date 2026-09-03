#include "vk_accel_structure_functions.h"

namespace imp::gfx::vulkan
{
	namespace
	{
		PFN_vkCreateAccelerationStructureKHR pfnCreateAccelerationStructureKHR = nullptr;
		PFN_vkDestroyAccelerationStructureKHR pfnDestroyAccelerationStructureKHR = nullptr;
		PFN_vkGetAccelerationStructureBuildSizesKHR pfnGetAccelerationStructureBuildSizesKHR = nullptr;
		PFN_vkCmdBuildAccelerationStructuresKHR pfnCmdBuildAccelerationStructuresKHR = nullptr;
		PFN_vkGetAccelerationStructureDeviceAddressKHR pfnGetAccelerationStructureDeviceAddressKHR = nullptr;
	}

	bool loadAccelStructFunctions(VkDevice device)
	{
		pfnCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
			vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR") );
		pfnDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
			vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR") );
		pfnGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
			vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR") );
		pfnCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
			vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR") );
		pfnGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
			vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR") );

		return pfnCreateAccelerationStructureKHR && pfnDestroyAccelerationStructureKHR
			&& pfnGetAccelerationStructureBuildSizesKHR && pfnCmdBuildAccelerationStructuresKHR 
			&& pfnGetAccelerationStructureDeviceAddressKHR;
	}

	VkResult vkCreateAccelerationStructureKHR_(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure)
	{
		return pfnCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
	}

	void vkDestroyAccelerationStructureKHR_(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks* pAllocator)
	{
		pfnDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
	}

	void vkGetAccelerationStructureBuildSizesKHR_(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const u32* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
	{
		pfnGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
	}

	void vkCmdBuildAccelerationStructuresKHR_(VkCommandBuffer commandBuffer, u32 infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
	{
		pfnCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
	}

	VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR_(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
	{
		return pfnGetAccelerationStructureDeviceAddressKHR(device, pInfo);
	}
}