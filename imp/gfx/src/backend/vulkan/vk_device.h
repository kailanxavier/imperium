#pragma once

#include <gfx/device.h>
#include <gfx/deferred_destroy_queue.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <optional>
#include <functional>
#include <vector>

#include "vk_buffer.h"

namespace imp::fwk { class Window; }
namespace imp::fs { class VirtualFileSystem; }
namespace imp::gfx::vulkan
{
	class VulkanSwapchain;
	class VulkanCommandContext;
	class VulkanCommandList;
	class VulkanRenderTarget;
	class VulkanDescriptorAllocator;
	class VulkanTexture;

	struct QueueFamilyIndices
	{
		std::optional<u32> graphics;
		std::optional<u32> present;
		[[nodiscard]] bool IsComplete() const { return graphics.has_value() && present.has_value(); }
	};

	class VulkanDevice final : public gfx::IDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice() override;

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

		bool initialise(const gfx::DeviceDesc& desc) override;
		void shutdown() override;

		[[nodiscard]] std::unique_ptr<gfx::IBuffer> createBuffer(const gfx::BufferDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IBlas> createBlas(const gfx::BlasBuildDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::ITlas> createTlas(const gfx::TlasBuildDesc& desc) override;

		[[nodiscard]] std::unique_ptr<gfx::ITexture> createTexture(const gfx::TextureDesc& desc) override;
		[[nodiscard]] std::vector<std::unique_ptr<ITexture>> createTextures(const std::vector<gfx::TextureDesc>& descs) override;

		[[nodiscard]] std::unique_ptr<gfx::ISampler> createSampler(const gfx::SamplerDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IShader> createShader(const gfx::ShaderDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IPipeline> createPipeline(const gfx::PipelineDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IPipeline> createComputePipeline(const gfx::ComputePipelineDesc& desc) override;

		[[nodiscard]] std::unique_ptr<gfx::IRenderTarget> createRenderTarget(const gfx::TextureDesc& desc) override;
		[[nodiscard]] std::vector<std::unique_ptr<IRenderTarget>> createCascadeRenderTargets(
			const TextureDesc& desc, ITexture** outArrayTexture) override;

		[[nodiscard]] u32 currentFrameIndex() const override;

		void waitIdle() override;

		bool initImGui() override;
		void shutdownImGui() override;
		void newImGuiFrame() override;
		void renderImGui(ICommandList &cmd) override;

		gfx::IRenderTarget& backBuffer() override;
		gfx::IRenderTarget* depthBuffer() override;
		
		gfx::ICommandList* beginFrame() override;
		void endFrame() override;

		gfx::GraphicsApi api() const override { return gfx::GraphicsApi::Vulkan; }
		const char* apiName() const override { return "Vulkan"; }

		[[nodiscard]] bool supportsRayTracing() const override { return m_rayQuerySupported; }

		[[nodiscard]] const fs::VirtualFileSystem& getVfs() const { return *m_vfs; }
	private:
		bool createInstance(const gfx::DeviceDesc& desc);
		bool setupDebugMessenger();
		bool createSurface(fwk::Window* window);
		bool pickPhysicalDevice();
		bool createLogicalDevice();
		bool createAllocator();
		bool createSwapchainInternal(const gfx::DeviceDesc& desc);
		bool createCommandsInternal();
		bool createDescriptorAllocatorInternal();

		void generateMipmaps(VkCommandBuffer cmd, VkImage image, VkFormat format, u32 width, u32 height, u32 mipLevels);

		bool submitOneTimeCommands(const std::function<void(VkCommandBuffer)>& record);
		bool readFileBytes(const std::string& path, std::vector<u8>& outBytes) const;

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		[[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device) const;
		[[nodiscard]] std::vector<const char*> getRequiredInstanceExtensions(bool wantValidation) const;

		[[nodiscard]] const VkAllocationCallbacks* allocationCallbacks() const
		{
			return m_hasHostAllocationCallbacks ? &m_hostAllocationCallbacks : nullptr;
		}

		bool ensureStagingBuffer(VkDeviceSize minSize);

		void deferredDestroy(std::function<void()> deleter) override;
		bool readbackTexture(IRenderTarget& target, std::vector<u8>& outPixels) override;

		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;
		VulkanBuffer m_stagingBuffer;

		std::unique_ptr<VulkanSwapchain> m_swapchain;
		std::unique_ptr<VulkanCommandContext> m_commands;
		std::unique_ptr<VulkanDescriptorAllocator> m_descriptorAllocator;
		std::unique_ptr<VulkanRenderTarget> m_backBufferTarget;
		std::unique_ptr<VulkanRenderTarget> m_depthBufferTarget;
		std::unique_ptr<VulkanCommandList> m_commandList;

		VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

		VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
		bool m_imguiInitialised = false;

		VkAllocationCallbacks m_hostAllocationCallbacks{};
		bool m_hasHostAllocationCallbacks = false;

		QueueFamilyIndices m_queueFamilies;
		bool m_validationEnabled = false;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_minimised = false;
		bool m_frameActive = false;

		bool m_anisotropySupported = false;
		float m_maxSamplerAnisotropy = 1.f;

		bool m_rayQuerySupported = false;
		std::vector<const char*> m_enabledDeviceExtensions;

		const fs::VirtualFileSystem* m_vfs = nullptr;

		gfx::DeferredDestroyQueue m_destroyQueue;
	};
}
