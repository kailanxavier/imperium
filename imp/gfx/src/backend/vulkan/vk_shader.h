#pragma once

#include <gfx/pipeline.h>

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include <core/fs/vfs.h>

namespace imp::gfx::vulkan
{
	VkShaderStageFlagBits toVkShaderStage(gfx::ShaderStage stage);

	struct ReflectedBinding
	{
		u32 binding = 0;
		u32 descriptorCount = 1;
		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		std::string name;
	};

	class VulkanShaderModule final : public gfx::IShader
	{
	public:
		VulkanShaderModule() = default;
		~VulkanShaderModule() override;

		VulkanShaderModule(const VulkanShaderModule&) = delete;
		VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

		VulkanShaderModule(VulkanShaderModule&& other) noexcept;
		VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

		bool loadFromBytes(VkDevice device, gfx::ShaderStage stage, const std::vector<u8>& code,
			const VkAllocationCallbacks* allocationCallbacks = nullptr);

		[[deprecated("loadFromFile() has been deprecated. Use loadFromBytes() instead")]]
		bool loadFromFile(VkDevice device, gfx::ShaderStage stage, const fs::VirtualFileSystem& vfs, const fs::Path& path, const VkAllocationCallbacks* allocationCallbacks);

		void destroy();

		[[nodiscard]] VkShaderModule handle() const { return m_module; }
		[[nodiscard]] bool isValid() const { return m_module != VK_NULL_HANDLE; }
		[[nodiscard]] const std::vector<ReflectedBinding>& reflectedBindings() const { return m_bindings; }

		[[nodiscard]] ShaderStage stage() const override { return m_stage; }

	private:
		bool reflect();

		VkDevice m_device = VK_NULL_HANDLE;
		VkShaderModule m_module = VK_NULL_HANDLE;
		ShaderStage m_stage = ShaderStage::Vertex;
		const VkAllocationCallbacks* m_allocationCallbacks = nullptr;

		std::vector<u32> m_spirvWords;
		std::vector<ReflectedBinding> m_bindings;
	};
}
