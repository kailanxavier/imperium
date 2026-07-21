#pragma once

#include <gfx/resources.h>
#include <vulkan/vulkan.h>

namespace imp::gfx::vulkan
{
	class VulkanSampler final : public gfx::ISampler
	{
	public:
		VulkanSampler() = default;
		~VulkanSampler() override;

		VulkanSampler(const VulkanSampler&) = delete;
		VulkanSampler& operator=(const VulkanSampler&) = delete;

		bool create(VkDevice device, const gfx::SamplerDesc& desc, const VkAllocationCallbacks* allocationCallbacks = nullptr);
		void destroy();

		[[nodiscard]] VkSampler handle() const { return m_sampler; }
		[[nodiscard]] bool isValid() const { return m_sampler != VK_NULL_HANDLE; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
