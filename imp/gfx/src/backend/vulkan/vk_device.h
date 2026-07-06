#pragma once

#include <memory>
#include <fwk/gfx_device.h>

#include <vulkan/vulkan.h>

#include <core/memory/int_types.h>

#include <optional>
#include <vector>

namespace imp::gfx::vulkan
{
	class VulkanSwapchain;
	class VulkanCommandContext;
	class VulkanGraphicsPipeline;

	struct QueueFamilyIndices
	{
		std::optional<u32> graphics;
		std::optional<u32> present;

		[[nodiscard]] bool IsComplete() const { return graphics.has_value() && present.has_value(); }
	};

	class VulkanDevice final : public fwk::IGfxDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice() override;

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

		bool initialise(const fwk::GfxDeviceDesc& desc) override;
		void shutdown() override;

		void onResize(u32 width, u32 height) override;
		void onMinimiseChanged(bool minimised) override;
		
		void beginFrame() override;
		void endFrame() override;

		[[nodiscard]] fwk::GfxApi getApi() const override { return fwk::GfxApi::Vulkan; }
		[[nodiscard]] const char* getApiName() const override { return "Vulkan"; }

		// Exposed for later pieces (swapchain module, renderer) that will
		// need the raw helpers. Kept here rather than made private so this
		// device can act as the shared context other Vulkan-side classes
		// build on, without everything being crammed into one file.
		[[nodiscard]] VkInstance getInstance() const { return m_instance; }
		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		[[nodiscard]] VkDevice getDevice() const { return m_device; }
		[[nodiscard]] VkSurfaceKHR getSurface() const { return m_surface; }
		[[nodiscard]] VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
		[[nodiscard]] VkQueue getPresentQueue() const { return m_presentQueue; }
		[[nodiscard]] const QueueFamilyIndices& getQueueFamilies() const { return m_queueFamilies; }

		[[nodiscard]] VulkanSwapchain* getSwapchain() const { return m_swapchain.get(); }
	private:
		bool createInstance(const fwk::GfxDeviceDesc& desc);
		bool setupDebugMessenger();
		bool createSurface(fwk::Window* window);
		bool pickPhysicalDevice();
		bool createLogicalDevice();
		bool createSwapchain(const fwk::GfxDeviceDesc& desc);
		bool createCommands();
		bool createPipeline();

		void recordAndSubmitFrame();

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		[[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device) const;
		[[nodiscard]] std::vector<const char*> getRequiredInstanceExtensions(bool wantValidation) const;

		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;

		std::unique_ptr<VulkanSwapchain> m_swapchain;
		std::unique_ptr<VulkanCommandContext> m_commands;
		std::unique_ptr<VulkanGraphicsPipeline> m_pipeline;

		QueueFamilyIndices m_queueFamilies;
		bool m_validationEnabled = false;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_minimised = false;

		bool m_frameActive = false;
	};
}
