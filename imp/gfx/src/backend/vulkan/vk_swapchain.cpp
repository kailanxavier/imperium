#include "vk_swapchain.h"

#include <core/log/log.h>

#include <algorithm>
#include <limits>

namespace imp::gfx::vulkan
{
    VulkanSwapchain::~VulkanSwapchain()
    {
        destroy();
    }

    bool VulkanSwapchain::create(const VulkanSwapchainCreateInfo& info)
    {
        m_physicalDevice = info.physicalDevice;
        m_device = info.device;
        m_surface = info.surface;
        m_graphicsQueueFamily = info.graphicsQueueFamily;
        m_presentQueueFamily = info.presentQueueFamily;
        m_graphicsQueue = info.graphicsQueue;
        m_presentQueue = info.presentQueue;
        m_vsync = info.vsync;

        if (!createSyncObjects())
            return false;

        // A 0 by 0 request is not an error, we should just start in
        // a not ready state, meaning beginFrame will return false
        // until we get a real size from resize()
        if (info.width == 0 || info.height == 0)
            return true;

        return createSwapchain(info.width, info.height, VK_NULL_HANDLE);
    }

    void VulkanSwapchain::destroy()
    {
        if (m_device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(m_device);
        destroySwapchainResources();
        destroySyncObjects();

        m_device = VK_NULL_HANDLE;
    }

    void VulkanSwapchain::resize(u32 width, u32 height)
    {
        m_pendingWidth = width;
        m_pendingHeight = height;
        m_needsRecreate = true;
    }

    bool VulkanSwapchain::beginFrame()
    {
        if (m_needsRecreate) recreate();
        if (!isReady()) return false;

        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<u64>::max());
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain,
            std::numeric_limits<u64>::max(),
            m_imageAvailableSemaphores[m_currentFrame],
            VK_NULL_HANDLE, &m_currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Surface capabilities changed enough that we can't present
            // into this swapchain at all anymore. Rebuild against the last
            // known size and let the called retry next frame.
            m_needsRecreate = true;
            m_pendingWidth = m_extent.width;
            m_pendingHeight = m_extent.height;
            return false;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            LOG_ERROR("Vulkan", "vkAcquireNextImageKHR failed ({})", static_cast<int>(result));
            return false;
        }

        // Only reset the fence once we know we're actually going to
        // render + submit this frame - resetting on a path that returns
        // false would leave the fence permanently unsignaled.
        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
        return true;
    }

    void VulkanSwapchain::present()
    {
        VkSemaphore waitSemaphores[] = { m_renderFinishedSemaphores[m_currentImageIndex] };
        VkSwapchainKHR swapchains[] = { m_swapchain };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &m_currentImageIndex;

        VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            m_needsRecreate = true;
            m_pendingWidth = m_extent.width;
            m_pendingHeight = m_extent.height;
        }
        else if (result != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan", "vkQueuePresentKHR failed ({})", static_cast<int>(result));
        }

        m_currentFrame = (m_currentFrame + 1) % kMaxFramesInFlight;
    }

    void VulkanSwapchain::recreate()
    {
        m_needsRecreate = false;

        if (m_pendingWidth == 0 || m_pendingHeight == 0)
        {
            // Window is minimised (or about to be) - tear down the swapchain
            // and sit idle rather than trying to create a 0 by 0 one.
            vkDeviceWaitIdle(m_device);
            destroySwapchainResources();
            return;
        }

        vkDeviceWaitIdle(m_device);

        VkSwapchainKHR old = m_swapchain;
        bool ok = createSwapchain(m_pendingWidth, m_pendingHeight, old);

        if (old != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(m_device, old, nullptr);

        if (!ok)
            LOG_ERROR("Vulkan", "Swapchain recreate failed.");
    }

    bool VulkanSwapchain::createSwapchain(u32 width, u32 height, VkSwapchainKHR oldSwapchain)
    {
        SupportDetails support = querySupport();
        if (support.formats.empty() || support.presentModes.empty())
        {
            LOG_ERROR("Vulkan", "Surface has no formats/present modes");
            return false;
        }

        VkSurfaceFormatKHR surfaceFormat = chooseFormat(support.formats);
        VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
        VkExtent2D extent = chooseExtent(support.capabilities, width, height);

        u32 imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
            imageCount = support.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        // COLOR_ATTACHMENT is enough since we're rendering directly with
        // dynamic rendering, no separate blit/copy target needed.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        u32 queueFamilyIndices[] = { m_graphicsQueueFamily, m_presentQueueFamily };
        if (m_graphicsQueueFamily != m_presentQueueFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan", "vkCreateSwapchainKHR failed.");
            return false;
        }

        // Tear down the previous images/views (but not the old
        // VkSwapchainKHR itself - the caller destroy that only after
        // this call returns, since oldSwapchain must stay valid until
        // vkCreateSwapchainKHR above completes).
        destroySwapchainResources();

        m_swapchain = newSwapchain;
        m_imageFormat = surfaceFormat.format;
        m_extent = extent;

        u32 actualCount = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualCount, nullptr);
        m_images.resize(actualCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualCount, m_images.data());

        if (!createImageViews()) return false;

        if (actualCount != m_renderFinishedSemaphores.size())
        {
            for (VkSemaphore s : m_renderFinishedSemaphores)
                if (s != VK_NULL_HANDLE) vkDestroySemaphore(m_device, s, nullptr);

            m_renderFinishedSemaphores.assign(actualCount, VK_NULL_HANDLE);
            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            for (u32 i = 0; i < actualCount; ++i)
            {
                if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
                {
                    LOG_ERROR("Vulkan", "vkCreateSemaphore (renderFinished) failed.");
                    return false;
                }
            }
        }

        m_currentImageIndex = 0;
        return true;
    }

    bool VulkanSwapchain::createImageViews()
    {
        m_imageViews.resize(m_images.size());
        for (size_t i = 0; i < m_images.size(); ++i)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_imageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
            {
                LOG_ERROR("Vulkan", "vkCreateImageView failed.");
                return false;
            }
        }

        return true;
    }

    bool VulkanSwapchain::createSyncObjects()
    {
        m_imageAvailableSemaphores.resize(kMaxFramesInFlight);
        m_inFlightFences.resize(kMaxFramesInFlight);

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (u32 i = 0; i < kMaxFramesInFlight; ++i)
        {
            if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS
                || vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            {
                LOG_ERROR("Vulkan", "Failed to create per-frame sync objects");
                return false;
            }
        }

        return true;
    }

    void VulkanSwapchain::destroySwapchainResources()
    {
        for (VkImageView view : m_imageViews)
            if (view != VK_NULL_HANDLE) vkDestroyImageView(m_device, view, nullptr);

        m_imageViews.clear();
        m_images.clear();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
    }

    void VulkanSwapchain::destroySyncObjects()
    {
        // Image available semaphores
        for (VkSemaphore s : m_imageAvailableSemaphores)
            if (s != VK_NULL_HANDLE) vkDestroySemaphore(m_device, s, nullptr);

        // Render finished semaphores
        for (VkSemaphore s : m_renderFinishedSemaphores)
            if (s != VK_NULL_HANDLE) vkDestroySemaphore(m_device, s, nullptr);

        // In flight fences
        for (VkFence f : m_inFlightFences)
            if (f != VK_NULL_HANDLE) vkDestroyFence(m_device, f, nullptr);

        m_imageAvailableSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_inFlightFences.clear();
    }

    VulkanSwapchain::SupportDetails VulkanSwapchain::querySupport() const
    {
        SupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &details.capabilities);

        u32 formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, details.formats.data());

        u32 presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, details.presentModes.data());

        return details;
    }

    VkSurfaceFormatKHR VulkanSwapchain::chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        for (const auto& f : formats)
        {
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        }
        return formats[0];
    }

    VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const
    {
        if (m_vsync)
            return VK_PRESENT_MODE_FIFO_KHR;

        for (VkPresentModeKHR m : modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;

        for (VkPresentModeKHR m : modes)
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps, u32 width, u32 height)
    {
        if (caps.currentExtent.width != std::numeric_limits<u32>::max())
            return caps.currentExtent;

        VkExtent2D extent { .width = width, .height = height };
        extent.width  = std::clamp(extent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
        extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        return extent;
    }
}