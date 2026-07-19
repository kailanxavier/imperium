#if defined(IMP_GFX_VULKAN)

#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_commands.h"
#include "vk_command_list.h"
#include "vk_render_target.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"
#include "vk_shader.h"
#include "vk_allocator.h"

#include <fwk/window.h>

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

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT,
			const VkDebugUtilsMessengerCallbackDataEXT* data,
			void*)
		{
			if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				LOG_ERROR("Vulkan", "{}", data->pMessage);

			return VK_FALSE;
		}

		VkResult createDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
			const VkAllocationCallbacks* allocator,
			VkDebugUtilsMessengerEXT* messenger)
		{
			auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") );

			if (func) return func(instance, createInfo, allocator, messenger);

			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
			const VkAllocationCallbacks* allocator)
		{
			auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT") );

			if (func) func(instance, messenger, allocator);
		}

		VkCullModeFlags toVkCullMode(gfx::CullMode mode)
		{
			switch (mode)
			{
			case imp::gfx::CullMode::None: return VK_CULL_MODE_NONE;
			case imp::gfx::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
			case imp::gfx::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
			}
			return VK_CULL_MODE_NONE;
		}

		VkCompareOp toVkCompareOp(gfx::CompareOp op)
		{
			switch (op)
			{
			case imp::gfx::CompareOp::Never: return VK_COMPARE_OP_NEVER;
			case imp::gfx::CompareOp::Less: return VK_COMPARE_OP_LESS;
			case imp::gfx::CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
			case imp::gfx::CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case imp::gfx::CompareOp::Greater: return VK_COMPARE_OP_GREATER;
			case imp::gfx::CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
			case imp::gfx::CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case imp::gfx::CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
			}
			return VK_COMPARE_OP_LESS;
		}

		VkFormat toVkAttributeFormat(u32 componentCount, bool isFloat)
		{
			if (!isFloat) return VK_FORMAT_UNDEFINED;
			switch (componentCount)
			{
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}

			return VK_FORMAT_UNDEFINED;
		}

		VkBufferUsageFlags toVkBufferUsage(gfx::BufferUsage usage)
		{
			VkBufferUsageFlags flags = 0;
			if (gfx::hasFlag(usage, gfx::BufferUsage::Vertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			if (gfx::hasFlag(usage, gfx::BufferUsage::Index)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if (gfx::hasFlag(usage, gfx::BufferUsage::Uniform)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			if (gfx::hasFlag(usage, gfx::BufferUsage::Storage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			return flags;
		}
	}

	VulkanDevice::VulkanDevice() = default;
	VulkanDevice::~VulkanDevice()
	{
		shutdown();
	}

	bool VulkanDevice::initialise(const gfx::DeviceDesc& desc)
	{
		m_vfs = desc.vfs;

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
		if (!createSwapchainInternal(desc)) { shutdown(); return false; }
		if (!createCommandsInternal()) { shutdown(); return false; }

		m_backBufferTarget = std::make_unique<VulkanRenderTarget>(*m_swapchain, VulkanRenderTargetKind::Colour);
		m_depthBufferTarget = std::make_unique<VulkanRenderTarget>(*m_swapchain, VulkanRenderTargetKind::Depth);
		m_commandList = std::make_unique<VulkanCommandList>();

		desc.window->setResizeCallback([this](u32 width, u32 height, bool minimised)
			{
				m_width = width;
				m_height = height;
				m_minimised = minimised;
				if (m_swapchain && !minimised)
					m_swapchain->resize(width, height);
			});

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
		LOG_INFO("Vulkan", "Device ready: {}", props.deviceName);

		return true;
	}

	void VulkanDevice::shutdown()
	{
		// All of these must go before the device
		m_commandList.reset();
		m_backBufferTarget.reset();
		m_depthBufferTarget.reset();
		m_commands.reset();
		m_swapchain.reset();

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
			destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, allocationCallbacks());
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

	std::unique_ptr<gfx::IBuffer> VulkanDevice::createBuffer(const gfx::BufferDesc& desc)
	{
		VulkanBufferCreateInfo info{};
		info.allocator = m_vmaAllocator;
		info.size = desc.size;
		info.usage = toVkBufferUsage(desc.usage);
		info.hostVisible = ( desc.memoryAccess == gfx::MemoryAccess::HostVisible );

		auto buffer = std::make_unique<VulkanBuffer>();
		if (!buffer->create(info))
		{
			LOG_ERROR("Vulkan", "creatBuffer failed");
			return nullptr;
		}

		return buffer;
	}

	std::unique_ptr<gfx::ITexture> VulkanDevice::createTexture(const gfx::TextureDesc& desc)
	{
		LOG_ERROR("Vulkan", "createTexture not implemented yet");
		return nullptr;
	}

	std::unique_ptr<gfx::ISampler> VulkanDevice::createSampler(const gfx::SamplerDesc& desc)
	{
		LOG_ERROR("Vulkan", "createSampler not implemented yet");
		return nullptr;
	}

	std::unique_ptr<gfx::IShader> VulkanDevice::createShader(const gfx::ShaderDesc& desc)
	{
		auto shader = std::make_unique<VulkanShaderModule>();
		if (!shader->loadFromFile(m_device, desc.stage, *m_vfs, desc.path, allocationCallbacks()))
		{
			LOG_ERROR("Vulkan", "createShader failed");
			return nullptr;
		}
		return shader;
	}

	std::unique_ptr<gfx::IPipeline> VulkanDevice::createPipeline(const gfx::PipelineDesc& desc)
	{
		if (!desc.vertexShader || !desc.fragmentShader)
		{
			LOG_ERROR("Vulkan", "createPipeline requires both a vertex and a fragment shader");
			return nullptr;
		}

		VulkanGraphicsPipelineCreateInfo info{};
		info.device = m_device;
		info.vertexShader = static_cast<VulkanShaderModule*>( desc.vertexShader )->handle();
		info.fragmentShader = static_cast<VulkanShaderModule*>( desc.fragmentShader )->handle();

		info.vertexBinding.binding = 0;
		info.vertexBinding.stride = desc.vertexLayout.stride;
		info.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		info.vertexAttributes.reserve(desc.vertexLayout.attributeCount);
		for (u32 i = 0; i < desc.vertexLayout.attributeCount; ++i)
		{
			const gfx::VertexAttribute& src = desc.vertexLayout.attributes[i];
			VkVertexInputAttributeDescription attr{};
			attr.location = src.location;
			attr.binding = 0;
			attr.offset = src.offset;
			attr.format = toVkAttributeFormat(src.componentCount, src.isFloat);
			info.vertexAttributes.push_back(attr);
		}

		info.vfs = m_vfs;
		info.cullMode = toVkCullMode(desc.rasterizerState.cullMode);
		info.depthTestEnable = desc.depthStencilState.depthTestEnable;
		info.depthWriteEnable = desc.depthStencilState.depthWriteEnable;
		info.depthCompareOp = toVkCompareOp(desc.depthStencilState.depthCompareOp);
		info.colourAttachmentFormat = toVkFormat(desc.colourFormat);
		info.depthAttachmentFormat = toVkFormat(desc.depthFormat);
		info.pushConstantSize = desc.pushConstantSize;
		info.allocationCallbacks = allocationCallbacks();

		auto pipeline = std::make_unique<VulkanGraphicsPipeline>();
		if (!pipeline->create(info))
		{
			LOG_ERROR("Vulkan", "createPipeline failed");
			return nullptr;
		}

		return pipeline;
	}

	gfx::IRenderTarget& VulkanDevice::backBuffer()
	{
		return *m_backBufferTarget;
	}

	gfx::IRenderTarget* VulkanDevice::depthBuffer()
	{
		return m_depthBufferTarget.get();
	}

	gfx::ICommandList* VulkanDevice::beginFrame()
	{
		m_frameActive = false;

		if (!m_swapchain || !m_commands) return nullptr;
		if (!m_swapchain->beginFrame()) return nullptr;	

		u32 frame = m_swapchain->currentFrameIndex();
		VkCommandBuffer cmd = m_commands->beginRecording(frame);
		m_commandList->reset(cmd);
		m_frameActive = true;

		return m_commandList.get();
	}

	void VulkanDevice::endFrame()
	{
		if (!m_frameActive) return;

		u32 frame = m_swapchain->currentFrameIndex();
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
		cmdSubmitInfo.commandBuffer = m_commandList->commandBuffer();

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

		m_swapchain->present();
		m_frameActive = false;
	}

	bool VulkanDevice::createInstance(const gfx::DeviceDesc& desc)
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
		appInfo.pEngineName = "IMPERIUM";
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

			debugCreateInfo.pfnUserCallback = debugCallback;
			createInfo.pNext = &debugCreateInfo;
		}

		if (vkCreateInstance(&createInfo, allocationCallbacks(), &m_instance) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "vkCreateInstance failed");
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

		createInfo.pfnUserCallback = debugCallback;

		if (createDebugUtilsMessengerEXT(m_instance, &createInfo, allocationCallbacks(), &m_debugMessenger) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "Failed to set up debug messenger");
			return false;
		}
		return true;
	}

	bool VulkanDevice::createSurface(fwk::Window* window)
	{
		if (glfwCreateWindowSurface(m_instance, window->getNativeHandle(), allocationCallbacks(), &m_surface) != VK_SUCCESS)
		{
			LOG_ERROR("Vulkan", "glfwCreateWindowSurface() failed");
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

	bool VulkanDevice::createSwapchainInternal(const gfx::DeviceDesc& desc)
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
		info.allocator = m_vmaAllocator;
		info.allocationCallbacks = allocationCallbacks();

		m_swapchain = std::make_unique<VulkanSwapchain>();
		if (!m_swapchain->create(info))
		{
			LOG_ERROR("Vulkan", "Failed to create swapchain");
			m_swapchain.reset();
			return false;
		}
		return true;
	}

	bool VulkanDevice::createCommandsInternal()
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

	QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) const
	{
		QueueFamilyIndices indices;
		u32 count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

		for (u32 i = 0; i < count; ++i)
		{
			if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphics = i;
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
			if (presentSupport) indices.present = i;
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

#endif // IMP_GFX_VULKAN
