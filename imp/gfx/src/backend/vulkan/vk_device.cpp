#if defined(IMP_GFX_VULKAN)

#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_commands.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_allocator.h"
#include "vk_vertex.h"
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

		if (desc.allocator)
		{
			m_hostAllocationCallbacks = makeVulkanAllocationCallbacks(*desc.allocator);
			m_hasHostAllocationCallbacks = true;
		}

		if (!createInstance(desc)) { shutdown(); return false; }
		if (m_validationEnabled && !setupDebugMessenger()) { shutdown(); return false; }
		if (!createSurface(desc.window)) { shutdown(); return false; }
		if (!pickPhysicalDevice()) { shutdown(); return false; }
		if (!createLogicalDevice()) { shutdown(); return false; }
		if (!createAllocator()) { shutdown(); return false; }
		if (!createSwapchain(desc)) { shutdown(); return false; }
		if (!createCommands()) { shutdown(); return false; }
		if (!createPipeline()) { shutdown(); return false; }
		if (!createVertexBuffer()) { shutdown(); return false; }

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
		LOG_INFO("Vulkan", "Device ready: {}", props.deviceName);

		return true;
	}

	void VulkanDevice::shutdown()
	{
		// Get rid of the stuff that belongs to device first
		// I cba adding a new comment to this every time something
		// belongs to m_device.
		m_pipeline.reset();
		m_commands.reset();
		m_swapchain.reset();

		m_vertexBuffer.reset();
		if (m_vmaAllocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(m_vmaAllocator);
			m_vmaAllocator = VK_NULL_HANDLE;
		}
		if (m_device)
		{
			vkDeviceWaitIdle(m_device);
			vkDestroyDevice(m_device, allocationCallbacks());
			m_device = VK_NULL_HANDLE;
		}
		if (m_debugMessenger)
		{
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, allocationCallbacks());
			m_debugMessenger = VK_NULL_HANDLE;
		}
		if (m_surface)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, allocationCallbacks());
			m_surface = VK_NULL_HANDLE;
		}
		if (m_instance)
		{
			vkDestroyInstance(m_instance, allocationCallbacks());
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
		m_frameActive = false;

		if (!m_swapchain || !m_commands) return;
		if (!m_swapchain->beginFrame()) return;

		m_frameActive = true;
	}

	void VulkanDevice::endFrame()
	{
		if (!m_frameActive) return;

		recordAndSubmitFrame();
		m_swapchain->present();
		m_frameActive = false;
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
		appInfo.apiVersion = VK_API_VERSION_1_3;

		auto extensions = getRequiredInstanceExtensions(m_validationEnabled);

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<u32>( extensions.size() );
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_validationEnabled)
		{
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;

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

		VkResult result = vkCreateInstance(&createInfo, allocationCallbacks(), &m_instance);
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

		if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, allocationCallbacks(), &m_debugMessenger) != VK_SUCCESS)
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
		VkResult result = glfwCreateWindowSurface(m_instance, window->getNativeHandle(), allocationCallbacks(), &m_surface);
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

		VkPhysicalDeviceVulkan13Features features13{}; // nothing yet, but I guess shit like ray tracing and dat goes here?
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &features13;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &features2;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<u32>( queueCreateInfos.size() );
		createInfo.pEnabledFeatures = VK_NULL_HANDLE;
		createInfo.enabledExtensionCount = static_cast<u32>( kDeviceExtensions.size() );
		createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

		if (vkCreateDevice(m_physicalDevice, &createInfo, allocationCallbacks(), &m_device) != VK_SUCCESS)
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
		info.allocationCallbacks = allocationCallbacks();

		m_swapchain = std::make_unique<VulkanSwapchain>();
		if (!m_swapchain->create(info))
		{
			LOG_FATAL("Vulkan", "Failed to create swapchain");
			m_swapchain.reset();
			return false;
		}
		return true;
	}

	bool VulkanDevice::createCommands()
	{
		m_commands = std::make_unique<VulkanCommandContext>();
		if (!m_commands->create(m_device, m_queueFamilies.graphics.value(), allocationCallbacks()))
		{
			LOG_ERROR("Vulkan", "Failed to create command context");
			m_commands.reset();
			return false;
		}

		return true;
	}

	bool VulkanDevice::createPipeline()
	{
		VulkanGraphicsPipelineCreateInfo info{};
		info.device = m_device;
		info.vertexShaderPath = "shaders/triangle.vert.spv";
		info.fragmentShaderPath = "shaders/triangle.frag.spv";
		info.colourAttachmentFormat = m_swapchain->imageFormat();
		info.allocationCallbacks = allocationCallbacks();

		m_pipeline = std::make_unique<VulkanGraphicsPipeline>();
		if (!m_pipeline->create(info))
		{
			LOG_ERROR("Vulkan", "Failed to create graphics pipeline");
			m_pipeline.reset();
			return false;
		}

		return true;
	}

	bool VulkanDevice::createAllocator()
	{
		VmaAllocatorCreateInfo info{};
		info.instance = m_instance;
		info.physicalDevice = m_physicalDevice;
		info.device = m_device;
		info.vulkanApiVersion = VK_API_VERSION_1_3;
		info.pAllocationCallbacks = allocationCallbacks();

		if (vmaCreateAllocator(&info, &m_vmaAllocator) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vmaCreateAllocator failed");
			return false;
		}
		return true;
	}

	bool VulkanDevice::createVertexBuffer()
	{
		const Vertex vertices[] =
		{
			{ { 0.f, -0.5f }, { 1.f, 0.f, 0.f } },
			{ { 0.5f, 0.5f }, { 0.f, 1.f, 0.f } },
			{ { -0.5f, 0.5f }, { 0.f, 0.f, 1.f } }
		};

		VulkanBufferCreateInfo info{};
		info.allocator = m_vmaAllocator;
		info.size = sizeof(vertices);
		info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		info.hostVisible = true;

		m_vertexBuffer = std::make_unique<VulkanBuffer>();
		if (!m_vertexBuffer->create(info))
		{
			LOG_ERROR("Vulkan", "Failed to create vertex buffer...");
			m_vertexBuffer.reset();
			return false;
		}

		std::memcpy(m_vertexBuffer->mappedData(), vertices, sizeof(vertices));
		return true;
	}

	void VulkanDevice::recordAndSubmitFrame()
	{
		u32 frame = m_swapchain->currentFrameIndex();
		VkCommandBuffer cmd = m_commands->beginRecording(frame);

		VkImageSubresourceRange colourRange{};
		colourRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // why everything in american
		colourRange.levelCount = 1;
		colourRange.layerCount = 1;

		VkImageMemoryBarrier2 toAttachment{};
		toAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		toAttachment.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		toAttachment.srcAccessMask = VK_ACCESS_2_NONE;
		toAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		toAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		toAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		toAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toAttachment.image = m_swapchain->currentImage();
		toAttachment.subresourceRange = colourRange;

		VkDependencyInfo toAttachmentDep{};
		toAttachmentDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		toAttachmentDep.imageMemoryBarrierCount = 1;
		toAttachmentDep.pImageMemoryBarriers = &toAttachment;
		vkCmdPipelineBarrier2(cmd, &toAttachmentDep);

		VkRenderingAttachmentInfo colourAttachment{};
		colourAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colourAttachment.imageView = m_swapchain->currentImageView();
		colourAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachment.clearValue.color = { { 0.023153f, 0.000911f, 0.004391f, 1.0f } };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea = { { 0, 0 }, m_swapchain->extent() };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colourAttachment;

		vkCmdBeginRendering(cmd, &renderingInfo);

		if (m_pipeline && m_pipeline->isValid())
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline());

			VkExtent2D extent = m_swapchain->extent();
			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = 0.f;
			viewport.width = static_cast<float>( extent.width );
			viewport.height = static_cast<float>( extent.height );
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			vkCmdSetScissor(cmd, 0, 1, &scissor);

			if (m_vertexBuffer && m_vertexBuffer->isValid())
			{
				VkBuffer vertexBuffers[] = { m_vertexBuffer->handle() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

				vkCmdDraw(cmd, 3, 1, 0, 0);
			}
		}

		vkCmdEndRendering(cmd);

		VkImageMemoryBarrier2 toPresent = toAttachment;
		toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		toPresent.dstAccessMask = VK_ACCESS_2_NONE;
		toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkDependencyInfo toPresentDep{};
		toPresentDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		toPresentDep.imageMemoryBarrierCount = 1;
		toPresentDep.pImageMemoryBarriers = &toPresent;
		vkCmdPipelineBarrier2(cmd, &toPresentDep);

		m_commands->endRecording(frame);

		VkSemaphoreSubmitInfo waitInfo{};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitInfo.semaphore = m_swapchain->currentImageAvailableSemaphore();
		waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo signalInfo{};
		signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfo.semaphore = m_swapchain->currentRenderFinishedSemaphore();
		signalInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkCommandBufferSubmitInfo cmdSubmitInfo{};
		cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdSubmitInfo.commandBuffer = cmd;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalInfo;

		if (vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, m_swapchain->currentInFlightFence()) != VK_SUCCESS)
			LOG_ERROR("Vulkan", "vkQueueSubmit2 failed");
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

		if (!required.empty()) return false;

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(device, &props);
		if (props.apiVersion < VK_API_VERSION_1_3) return false;

		VkPhysicalDeviceVulkan13Features features13{};
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &features13;

		vkGetPhysicalDeviceFeatures2(device, &features2);
		if (!features13.dynamicRendering || !features13.synchronization2) return false;

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
