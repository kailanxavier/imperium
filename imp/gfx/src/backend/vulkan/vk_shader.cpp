#include "vk_shader.h"
#include "vk_check.h"

#include <core/log/log.h>
#include <core/types/int_types.h>

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

	bool VulkanShaderModule::loadFromBytes(VkDevice device, gfx::ShaderStage stage, const std::vector<u8>& code, const VkAllocationCallbacks* allocationCallbacks)
	{
		if (code.empty() || ( code.size() % 4 ) != 0)
		{
			LOG_ERROR("Vulkan", "Shader bytecode has invalid size, not a valid SPIR-V binary");
			return false;
		}

		// While every real allocator wouldn't have a problem here,
		// hypothetically the 4-byte alignment isn't promises to u8
		// in the standard. So for safety we will copy it into another
		// vector instead of casting it into u32 later
		std::vector<u32> alignedCode(code.size() / sizeof(u32));
		std::memcpy(alignedCode.data(), code.data(), code.size());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = alignedCode.data();

		VK_CHECK(vkCreateShaderModule(device, &createInfo, allocationCallbacks, &m_module));

		m_device = device;
		m_stage = stage;
		m_allocationCallbacks = allocationCallbacks;
		return true;
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

		VK_CHECK(vkCreateShaderModule(m_device, &createInfo, m_allocationCallbacks, &m_module));
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
