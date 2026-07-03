#if defined(IMP_GFX_VULKAN)

#include "vk_device.h"
#include "vk_swapchain.h"
#include <fwk/window.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <core/memory/int_types.h>
#include <core/log/log.h>

#include <cstring>
#include <set>

namespace imp::gfx::vulkan
{
	namespace
	{
		const std::vector<const char*> kValidationLayers = {
			"VK_LAYER_KHRONOS_validation",
		};

		const std::vector<const char*> kDeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		bool checkValidationLayerSupport()
		{
			u32 layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			std::vector<VkLayerProperties> available(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, available.data());

			for (const char* wanted : kValidationLayers)
			{
				bool found = false;
				for (const auto& layer : available)
				{
					if (std::strcmp(wanted, layer.layerName) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found) return false;
			}

			return true;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT,
			const VkDebugUtilsMessengerCallbackDataEXT* data,
			void*)
		{
			if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				LOG_ERROR("Vulkan", "{}", data->pMessage);

			return VK_FALSE;
		}

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
			const VkAllocationCallbacks* allocator,
			VkDebugUtilsMessengerEXT* messenger)
		{
			auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") );

			if (func) return func(instance, createInfo, allocator, messenger);

			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
			const VkAllocationCallbacks* allocator)
		{
			auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT") );

			if (func) func(instance, messenger, allocator);
		}
	} // namespace

	VulkanDevice::VulkanDevice()  = default;

	VulkanDevice::~VulkanDevice()
	{
		shutdown();
	}

	bool VulkanDevice::initialise(const fwk::GfxDeviceDesc& desc)
	{
		if (!desc.window)
		{
			LOG_ERROR("Vulkan", "initialise() requires a window");
			return false;
		}

		m_validationEnabled = desc.enableValidation;
		m_width = desc.window->width();
		m_height = desc.window->height();

		if (!createInstance(desc)) { shutdown(); return false; }
		if (m_validationEnabled && !setupDebugMessenger()) { shutdown(); return false; }
		if (!createSurface(desc.window)) { shutdown(); return false; }
		if (!pickPhysicalDevice()) { shutdown(); return false; }
		if (!createLogicalDevice()) { shutdown(); return false; }
		if (!createSwapchain(desc)) { shutdown(); return false; }

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
		LOG_INFO("Vulkan", "Device ready: {}", props.deviceName);

		return true;
	}

	void VulkanDevice::shutdown()
	{
		// Swapchain must go before the device itself
		m_swapchain.reset();

		if (m_device)
		{
			vkDeviceWaitIdle(m_device);
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
		if (m_debugMessenger)
		{
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
			m_debugMessenger = VK_NULL_HANDLE;
		}
		if (m_surface)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}
		if (m_instance)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}

		m_physicalDevice = VK_NULL_HANDLE;
		m_graphicsQueue = VK_NULL_HANDLE;
		m_presentQueue = VK_NULL_HANDLE;
	}

	void VulkanDevice::onResize(u32 width, u32 height)
	{
		m_width = width;
		m_height = height;

		if (m_swapchain)
			m_swapchain->resize(width, height);
	}

	void VulkanDevice::onMinimiseChanged(bool minimised)
	{
		m_minimised = minimised;
	}

	void VulkanDevice::beginFrame()
	{
		if (!m_swapchain)
			return;

		// if (!m_swapchain->beginFrame()) return;
			// record commands into m_swapchain->currentImageView()
	}

	void VulkanDevice::endFrame()
	{
		if (!m_swapchain)
			return;

		// m_swapchain-present();

		// TODO: Command buffer but for now this and begin
		// frame will be stubbed out
	}

	bool VulkanDevice::createInstance(const fwk::GfxDeviceDesc& desc)
	{
		if (m_validationEnabled && !checkValidationLayerSupport())
		{
			LOG_WARN("Vulkan", "Validation requested but layer not available; continuing without validating.");
			m_validationEnabled = false;
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = desc.appName;
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.pEngineName = "imperium_of_man";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		auto extensions = getRequiredInstanceExtensions(m_validationEnabled);

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<u32>( extensions.size() );
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_validationEnabled)
		{
			createInfo.enabledLayerCount = static_cast<u32>( kValidationLayers.size() );
			createInfo.ppEnabledLayerNames = kValidationLayers.data();

			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			debugCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			debugCreateInfo.pfnUserCallback = DebugCallback;

			// Chained so instance creation itself is also validated
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateInstance failed ({})", static_cast<int>( result ));
			return false;
		}

		return true;
	}

	bool VulkanDevice::setupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = DebugCallback;

		if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "Failed to set up debug messenger");
			return false;
		}

		return true;
	}

	bool VulkanDevice::createSurface(fwk::Window* window)
	{
		// fwk::Window is built with GLFW under the hood; this is the one
		// place that fact leaks into the Vulkan backend, via
		// GetNativeHandle(). fwk's own headers stay Vulkan free.
		VkResult result = glfwCreateWindowSurface(m_instance, window->getNativeHandle(), nullptr, &m_surface);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "glfwCreateWindowSurface() failed ({})", static_cast<int>( result ));
			return false;
		}
		return true;
	}

	bool VulkanDevice::pickPhysicalDevice()
	{
		u32 count = 0;
		vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
		if (count == 0)
		{
			LOG_FATAL("Vulkan", "No GPUs with Vulkan support were found.");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

		for (VkPhysicalDevice device : devices)
		{
			if (isDeviceSuitable(device))
			{
				m_physicalDevice = device;
				m_queueFamilies = findQueueFamilies(device);
				return true;
			}
		}

		LOG_FATAL("Vulkan", "No suitable GPU was found");
		return false;
	}

	bool VulkanDevice::createLogicalDevice()
	{
		std::set<u32> uniqueFamilies =
		{
			m_queueFamilies.graphics.value(),
			m_queueFamilies.present.value()
		};

		float priority = 1.f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (u32 family : uniqueFamilies)
		{
			VkDeviceQueueCreateInfo qci{};
			qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			qci.queueFamilyIndex = family;
			qci.queueCount = 1;
			qci.pQueuePriorities = &priority;
			queueCreateInfos.push_back(qci);
		}

		VkPhysicalDeviceFeatures features{}; // nothing yet, but I guess shit like ray tracing and dat goes here?

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<u32>( queueCreateInfos.size() );
		createInfo.pEnabledFeatures = &features;
		createInfo.enabledExtensionCount = static_cast<u32>( kDeviceExtensions.size() );
		createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

		if (m_validationEnabled)
		{
			// Ignored on modern loaders (device layers are deprecated), BUT
			// seems harmless enough to set for older loader compatibility
			createInfo.enabledLayerCount = static_cast<u32>( kValidationLayers.size() );
			createInfo.ppEnabledLayerNames = kValidationLayers.data();
		}

		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateDevice() failed");
			return false;
		}

		vkGetDeviceQueue(m_device, m_queueFamilies.graphics.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, m_queueFamilies.present.value(), 0, &m_presentQueue);
		return true;
	}

	bool VulkanDevice::createSwapchain(const fwk::GfxDeviceDesc &desc)
	{
		VulkanSwapchainCreateInfo info{};
		info.physicalDevice = m_physicalDevice;
		info.device = m_device;
		info.surface = m_surface;
		info.graphicsQueueFamily = m_queueFamilies.graphics.value();
		info.presentQueueFamily = m_queueFamilies.present.value();
		info.graphicsQueue = m_graphicsQueue;
		info.presentQueue = m_presentQueue;
		info.width = m_width;
		info.height = m_height;
		info.vsync = desc.vsync;

		m_swapchain = std::make_unique<VulkanSwapchain>();
		if (!m_swapchain->create(info))
		{
			LOG_FATAL("Vulkan", "Failed to create swapchain");
			m_swapchain.reset();
			return false;
		}
		return true;
	}

	QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices;

		u32 count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

		for (u32 i = 0; i < count; ++i)
		{
			if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphics = i;

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
			if (presentSupport)
				indices.present = i;

			if (indices.IsComplete()) break;
		}
		return indices;
	}

	bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices = findQueueFamilies(device);
		if (!indices.IsComplete()) return false;

		u32 extCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
		std::vector<VkExtensionProperties> available(extCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, available.data());

		std::set<std::string> required(kDeviceExtensions.begin(), kDeviceExtensions.end());
		for (const auto& ext : available)
			required.erase(ext.extensionName);

		if (!required.empty())
			return false;

		// Swapchain support check (formats/present modes) belongs here once
		// the swapchain module is written; for now, extension presence is
		// enough to prove the wiring.
		return true;
	}

	std::vector<const char*> VulkanDevice::getRequiredInstanceExtensions(bool wantValidation) const
	{
		u32 glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (wantValidation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}
}

#endif
