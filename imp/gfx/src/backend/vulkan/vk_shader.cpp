#include "vk_shader.h"

#include <core/log/log.h>
#include <core/memory/int_types.h>

#include <fstream>
#include <vector>

namespace imp::gfx::vulkan
{
	VulkanShaderModule::~VulkanShaderModule()
	{
		destroy();
	}

	VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
		: m_device(other.m_device), m_module(other.m_module)
	{
		other.m_device = VK_NULL_HANDLE;
		other.m_module = VK_NULL_HANDLE;
	}

	VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			m_device = other.m_device;
			m_module = other.m_module;
			other.m_device = VK_NULL_HANDLE;
			other.m_module = VK_NULL_HANDLE;
		}
		return *this;
	}

	bool VulkanShaderModule::loadFromFile(VkDevice device, gfx::ShaderStage stage,
		const fs::VirtualFileSystem& vfs,
		const fs::Path& path, const VkAllocationCallbacks* allocationCallbacks)
	{
		m_device = device;
		m_stage = stage;
		m_allocationCallbacks = allocationCallbacks;

		fs::Bytes raw;
		if (!vfs.readEntireFile(path, raw))
		{
			LOG_ERROR("Vulkan", "Failed to read shader file: {}", path.c_str());
			return false;
		}

		if (raw.empty() || (raw.size() % 4) != 0)
		{
			LOG_ERROR("Vulkan", "Shader file has invalid size, not a valid SPIR-V binary: {}", path.c_str());
			return false;
		}

		std::vector<u32> code(raw.size() / sizeof(u32));
		std::memcpy(code.data(), raw.data(), raw.size());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = raw.size();
		createInfo.pCode = code.data();

		if (vkCreateShaderModule(m_device, &createInfo, m_allocationCallbacks, &m_module) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateShaderModule failed {}", path.c_str());
			return false;
		}

		return true;
	}

	void VulkanShaderModule::destroy()
	{
		if (m_module != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_device, m_module, m_allocationCallbacks);
			m_module = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}
	
	VkShaderStageFlagBits toVkShaderStage(gfx::ShaderStage stage)
	{
		switch (stage)
		{
		case imp::gfx::ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case imp::gfx::ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case imp::gfx::ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		return VK_SHADER_STAGE_ALL;
	}

}
