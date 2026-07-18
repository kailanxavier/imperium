#pragma once

#include <vulkan/vulkan.h>
#include <string>

#include <core/fs/vfs.h>

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

		// Load SPIR-V bytecode through the engine's VFS rather than the OS
		// filesystem directly, so shader assets are subject to the same
		// mount/override rules as everything else (that is to come)
		bool loadFromFile(VkDevice device, const fs::VirtualFileSystem& vfs, const fs::Path& path, const VkAllocationCallbacks* allocationCallbacks);
		void destroy();

		[[nodiscard]] VkShaderModule handle() const { return m_module; }
		[[nodiscard]] bool isValid() const { return m_module != VK_NULL_HANDLE; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkShaderModule m_module = VK_NULL_HANDLE;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
