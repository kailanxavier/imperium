#pragma once

#include <vulkan/vulkan.h>
#include <string>

namespace imp::gfx::vulkan
{
	class VulkanShaderModule
	{
	public:
		VulkanShaderModule() = default;
		~VulkanShaderModule();

		VulkanShaderModule(const VulkanShaderModule&) = delete;
		VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

		VulkanShaderModule(VulkanShaderModule&& other) noexcept;
		VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

		// NOTE: this read directly off the OS filesystem rather than
		// through the engine's asset/VFS layer because there isn't one 
		// wired into gfx at the moment. If/when shader loading should
		// go through the VFS instead, this is the only place we will
		// have to change.
		bool loadFromFile(VkDevice device, const std::string& path, const VkAllocationCallbacks* allocationCallbacks = nullptr);
		void destroy();

		VkShaderModule handle() const { return m_module; }
		bool isValid() const { return m_module != VK_NULL_HANDLE; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkShaderModule m_module = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}