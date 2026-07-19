#pragma once

#include <gfx/pipeline.h>

#include <vulkan/vulkan.h>
#include <string>

#include <core/fs/vfs.h>

namespace imp::gfx::vulkan
{
	VkShaderStageFlagBits toVkShaderStage(gfx::ShaderStage stage);

	class VulkanShaderModule final : public gfx::IShader
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
		bool loadFromFile(VkDevice device, gfx::ShaderStage stage, const fs::VirtualFileSystem& vfs, const fs::Path& path, const VkAllocationCallbacks* allocationCallbacks);
		void destroy();

		[[nodiscard]] VkShaderModule handle() const { return m_module; }
		[[nodiscard]] bool isValid() const { return m_module != VK_NULL_HANDLE; }

		gfx::ShaderStage stage() const override { return m_stage; }

	private:
		VkDevice m_device = VK_NULL_HANDLE;
		VkShaderModule m_module = VK_NULL_HANDLE;
		gfx::ShaderStage m_stage = gfx::ShaderStage::Vertex;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;
	};
}
