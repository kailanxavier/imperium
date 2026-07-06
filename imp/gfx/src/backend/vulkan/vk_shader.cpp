#if defined(IMP_GFX_VULKAN)

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

	bool VulkanShaderModule::loadFromFile(VkDevice device, const std::string& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			LOG_ERROR("Vulkan", "Failed to open shader file: {}", path.c_str());
			return false;
		}

		std::streamsize size = file.tellg();
		if (size <= 0 || ( size % 4 ) != 0)
		{
			LOG_ERROR("Vulkan", "Shader file has invalid size, not a valid SPIR-V binary: {}", path.c_str());
			return false;
		}

		file.seekg(0, std::ios::beg);
		std::vector<u32> code(static_cast<size_t>( size ) / sizeof(u32));
		if (!file.read(reinterpret_cast<char*>( code.data() ), size))
		{
			LOG_ERROR("Vulkan", "Failed to read shader file {}: ", path.c_str());
			return false;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = static_cast<size_t>( size );
		createInfo.pCode = code.data();

		if (vkCreateShaderModule(device, &createInfo, nullptr, &m_module) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateShaderModule failed {}", path.c_str());
			return false;
		}

		m_device = device;
		return true;
	}

	void VulkanShaderModule::destroy()
	{
		if (m_module != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_device, m_module, nullptr);
			m_module = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}
	
}

#endif