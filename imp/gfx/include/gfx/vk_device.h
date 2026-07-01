#pragma once

#include <fwk/gfx_device.h>

#include <vulkan/vulkan.h>

#include <core/memory/int_types.h>
#include <optional>
#include <vector>

namespace imp::gfx::vulkan
{
	struct QueueFamilyIndices
	{
		std::optional<u32> graphics;
		std::optional<u32> present;

		bool IsComplete() const { return graphics.has_value() && present.has_value(); }
	};

	class VulkanDevice final : public fwk::IGfxDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice() override;

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

		bool initialise(const fwk::GfxDeviceDesc& desc) override;
		void shutdown() override;

		void onResize(u32 width, u32 height) override;
		void onMinimiseChanged(bool minimised) override;
		
		void beginFrame() override;
		void endFrame() override;

		fwk::GfxApi getApi() const override { return fwk::GfxApi::Vulkan; }
		const char* getApiName() const override { return "Vulkan"; }

		// Exposed for later pieces (swapchain module, renderer) that will
		// need the raw helpers. Kept here rather than made private so this
		// device can act as the shared context other Vulkan-side classes
		// build on, without everything being crammed into one file.
		VkInstance getInstance() const { return m_instance; }
		VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		VkDevice getDevice() const { return m_device; }
		VkSurfaceKHR getSurface() const { return m_surface; }
		VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
		VkQueue getPresentQueue() const { return m_presentQueue; }
		const QueueFamilyIndices& getQueueFamilies() const { return m_queueFamilies; }

	private:
		bool createInstance(const fwk::GfxDeviceDesc& desc);
		bool setupDebugMessenger();
		bool createSurface(fwk::Window* window);
		bool pickPhysicalDevice();
		bool createLogicalDevice();

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		bool isDeviceSuitable(VkPhysicalDevice device) const;
		std::vector<const char*> getRequiredInstanceExtensions(bool wantValidation) const;

		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;

		QueueFamilyIndices m_queueFamilies;
		bool m_validationEnabled = false;
		u32 m_width = 0;
		u32 m_height = 0;
		bool m_minimised = false;
	};
}
