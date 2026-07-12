#pragma once

#include <gfx/swapchain.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <core/memory/int_types.h>

#include "vk_config.h"

#include <vector>

namespace imp::gfx::vulkan
{
    struct VulkanSwapchainCreateInfo
    {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        u32 graphicsQueueFamily = 0;
        u32 presentQueueFamily = 0;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        u32 width = 0;
        u32 height = 0;
        bool vsync = true;

        VmaAllocator allocator = VK_NULL_HANDLE;
        const VkAllocationCallbacks* allocationCallbacks = nullptr;
    };

    class VulkanSwapchain final : public ISwapChain
    {
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain() override;

        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

        // Not part of ISwapchain: construction needs backend specific
        // handles the agnostic interface can't carry. Mirrors the
        // desc-struct pattern already used by IGfxDevice::initialise().
        bool create(const VulkanSwapchainCreateInfo& info);
        void destroy();

        void resize(u32 width, u32 height) override;
        bool beginFrame() override;
        void present() override;

        [[nodiscard]] u32 currentBackBufferIndex() const override { return m_currentImageIndex; }
        [[nodiscard]] u32 backBufferCount() const override { return static_cast<u32>(m_images.size()); }
        [[nodiscard]] u32 currentFrameIndex() const { return m_currentFrame; }

        // Vulkan specific accessors for whatever records commands
        // against the current back buffer (the currently unwritten render module)
        // Fine to expose raw handles here; this is a private backend header
        [[nodiscard]] VkExtent2D extent() const { return m_extent; }
        [[nodiscard]] VkFormat imageFormat() const { return m_imageFormat; }
        [[nodiscard]] VkImage currentImage() const { return m_images[m_currentImageIndex]; }
        [[nodiscard]] VkImageView currentImageView() const { return m_imageViews[m_currentImageIndex]; }

        [[nodiscard]] VkImageView depthImageView() const { return m_depthImageView; }
        [[nodiscard]] VkImage depthImage() const { return m_depthImage; }
        [[nodiscard]] VkFormat depthFormat() const { return m_depthFormat; }

        // Sync objects the caller must wait on / signal when submitting
        // the command buffer for this frame (see VulkanDevice::endFrame
        // once command submission exists)
        [[nodiscard]] VkSemaphore currentImageAvailableSemaphore() const { return m_imageAvailableSemaphores[m_currentFrame]; }
        [[nodiscard]] VkSemaphore currentRenderFinishedSemaphore() const { return m_renderFinishedSemaphores[m_currentImageIndex]; }
        [[nodiscard]] VkFence currentInFlightFence() const { return m_inFlightFences[m_currentFrame]; }

        [[nodiscard]] bool isReady() const { return m_swapchain != VK_NULL_HANDLE; }

    private:
        struct SupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities{};
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        bool createSwapchain(u32 width, u32 height, VkSwapchainKHR oldSwapchain);
        bool createImageViews();
        bool createSyncObjects();
        void destroySwapchainResources();
        void destroySyncObjects();
        void recreate();

        bool pickDepthFormat();
        bool createDepthResources();
        void destroyDepthResources();

        SupportDetails querySupport() const;
        static VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
        static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, u32 width, u32 height);

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        u32 m_graphicsQueueFamily = 0;
        u32 m_presentQueueFamily = 0;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        bool m_vsync = true;
        VmaAllocator m_allocator = VK_NULL_HANDLE;
        const VkAllocationCallbacks* m_allocationCallbacks = nullptr;

        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D m_extent{};
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;

        VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
        VkImage m_depthImage = VK_NULL_HANDLE;
        VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
        VkImageView m_depthImageView = VK_NULL_HANDLE;

        std::vector<VkSemaphore> m_imageAvailableSemaphores; // sized kMaxFramesInFlight
        std::vector<VkSemaphore> m_renderFinishedSemaphores; // sized to image count
        std::vector<VkFence>     m_inFlightFences;           // sized kMaxFramesInFlight

        u32 m_currentFrame = 0;
        u32 m_currentImageIndex = 0;

        bool m_needsRecreate = false;
        u32 m_pendingWidth = 0;
        u32 m_pendingHeight = 0;
    };
}