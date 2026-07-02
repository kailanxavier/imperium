#include "gfx/gfx.h"

#include <core/log/log.h>

#if defined(IMP_GFX_DX11)
	#include "../include/gfx/dx11_device.h"
#elif defined(IMP_GFX_DX12)
	#include "../include/gfx/dx12_device.h"
#elif defined(IMP_GFX_VULKAN)
	#include "../include/gfx/vk_device.h"
#else
	#error "No IMP_GFX defined"
#endif

namespace imp::gfx
{
	std::unique_ptr<fwk::IGfxDevice> createDevice()
	{
#if defined(IMP_GFX_DX11)
		LOG_INFO("Graphics", "Factory creating DX11 Device...");
		return std::make_unique<dx11::DX11Device>();

#elif defined(IMP_GFX_DX12)
		LOG_INFO("Graphics", "Factory creating DX12 Device...");
		return std::make_unique<dx12::DX12Device>();

#elif defined(IMP_GFX_VULKAN)
		LOG_INFO("Graphics", "Factory creating Vulkan Device...");
		return std::make_unique<vulkan::VulkanDevice>();
#endif
	}

	const char* Version() { return "imp_gfx 0.1.0"; }

	const char* ActiveBackend()
	{
#if defined(IMP_GFX_DX11)
		return "DX11";
#elif defined(IMP_GFX_DX12)
		return "DX12";
#elif defined(IMP_GFX_VULKAN)
		return "Vulkan";
#else
		return "None";
#endif
	}
}