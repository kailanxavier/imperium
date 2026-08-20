#include "vk_shader.h"
#include "vk_check.h"
#include <spirv_reflect.h>

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
		: m_device(other.m_device)
		, m_module(other.m_module)
		, m_stage(other.m_stage)
		, m_allocationCallbacks(other.m_allocationCallbacks)
		, m_spirvWords(std::move(other.m_spirvWords))
		, m_bindings(std::move(other.m_bindings))
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
			m_stage = other.m_stage;
			m_allocationCallbacks = other.m_allocationCallbacks;
			m_spirvWords = std::move(other.m_spirvWords);
			m_bindings = std::move(other.m_bindings);
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
		m_spirvWords = std::move(alignedCode);

		if (!reflect())
		{
			LOG_ERROR("Vulkan", "Shader reflection failed. Refused to treat this module as loaded!");
			destroy();
			return false;
		}

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

	bool VulkanShaderModule::reflect()
	{
		SpvReflectShaderModule module{};
		SpvReflectResult result = spvReflectCreateShaderModule(
			m_spirvWords.size() * sizeof(u32), m_spirvWords.data(), &module);

		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			LOG_ERROR("Vulkan", "spvReflectCreateShaderModule failed: ({})", static_cast<int>(result));
			return false;
		}

		u32 setCount = 0;
		spvReflectEnumerateDescriptorSets(&module, &setCount, nullptr);
		std::vector<SpvReflectDescriptorSet*> sets(setCount);
		spvReflectEnumerateDescriptorSets(&module, &setCount, sets.data());

		m_bindings.clear();
		for (const SpvReflectDescriptorSet* set : sets)
		{
			if (set->set != 0)
			{
				LOG_WARN("Vulkan", "Shader declares descriptor set {}. Only 0 supported, ignoring", set->set);
				continue;
			}

			for (u32 i = 0; i < set->binding_count; ++i)
			{
				const SpvReflectDescriptorBinding* b = set->bindings[i];

				ReflectedBinding rb{};
				rb.binding = b->binding;
				rb.descriptorType = static_cast<VkDescriptorType>( b->descriptor_type );
				rb.descriptorCount = ( b->count > 0 ) ? b->count : 1;
				rb.name = b->name ? b->name : "";
				m_bindings.push_back(rb);
			}
		}

		u32 pushConstantCount = 0;
		spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
		if (pushConstantCount > 0)
		{
			std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
			spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());

			if (pushConstantCount > 1)
				LOG_WARN("Vulkan", "Shader declares {} push constant blocks, expected at most 1. Using the last one.", pushConstantCount);

			m_pushConstantSize = pushConstants.back()->size;
		}
		else
		{
			m_pushConstantSize = 0;
		}

		spvReflectDestroyShaderModule(&module);
		return true;
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
