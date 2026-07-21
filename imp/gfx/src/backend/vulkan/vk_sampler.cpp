#if defined(IMP_GFX_VULKAN)
#include "vk_sampler.h"
#include <core/log/log.h>

namespace imp::gfx::vulkan
{
	namespace
	{
		VkFilter toVkFilter(gfx::FilterMode mode)
		{
			// no need for a switch here
			return mode == gfx::FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		}

		VkSamplerAddressMode toVkAddressMode(gfx::AddressMode mode)
		{
			switch (mode)
			{
			case imp::gfx::AddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case imp::gfx::AddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case imp::gfx::AddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			}
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}

	VulkanSampler::~VulkanSampler() { destroy(); }

	bool VulkanSampler::create(VkDevice device, const gfx::SamplerDesc& desc, const VkAllocationCallbacks* allocationCallbacks)
	{
		m_device = device;
		m_allocationCallbacks = allocationCallbacks;

		VkSamplerCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = toVkFilter(desc.magFilter);
		info.minFilter = toVkFilter(desc.minFilter);
		info.addressModeU = toVkAddressMode(desc.addressModeU);
		info.addressModeV = toVkAddressMode(desc.addressModeV);
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // no 3D/cubemap textures yet, but W MUST be valid
		info.anisotropyEnable = desc.enableAnisotropy ? VK_TRUE : VK_FALSE;
		info.maxAnisotropy = desc.enableAnisotropy ? 4.f : 1.f;
		info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		info.compareEnable = VK_FALSE;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(m_device, &info, m_allocationCallbacks, &m_sampler) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateSampler failed");
			return false;
		}

		return true;
	}

	void VulkanSampler::destroy()
	{
		if (m_sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_device, m_sampler, m_allocationCallbacks);
			m_sampler = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;
	}
}

#endif
