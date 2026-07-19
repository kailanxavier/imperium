#include "gfx/device.h"
#include <core/log/log.h>

#if defined(IMP_GFX_DX11)
	#include "backend/dx11/dx11_device.h"
#endif
#if defined(IMP_GFX_DX12)
	#include "backend/dx12/dx12_device.h"
#endif
#if defined(IMP_GFX_VULKAN)
	#include "backend/vulkan/vk_device.h"
#endif

namespace imp::gfx
{
	const char* toString(GraphicsApi api)
	{
		switch (api)
		{
		case imp::gfx::GraphicsApi::Vulkan: return "Vulkan";
		case imp::gfx::GraphicsApi::D3D12: return "DirectX 12";
		case imp::gfx::GraphicsApi::D3D11: return "DirectX 11";
		}
		return "Unknown";
	}

	bool isApiAvailable(GraphicsApi api)
	{
		switch (api)
		{
		case imp::gfx::GraphicsApi::Vulkan:
#if defined(IMP_GFX_VULKAN)
			return true;
#else
			return false;
#endif
		case imp::gfx::GraphicsApi::D3D12:
#if defined(IMP_GFX_DX12)
			return true;
#else
			return false;
#endif
		case imp::gfx::GraphicsApi::D3D11:
#if defined(IMP_GFX_DX11)
			return true;
#else
			return false;
#endif
		}
		return false;
	}

	std::vector<GraphicsApi> availableApis()
	{
		std::vector<GraphicsApi> apis;
		if (isApiAvailable(GraphicsApi::Vulkan)) apis.push_back(GraphicsApi::Vulkan);
		if (isApiAvailable(GraphicsApi::D3D11)) apis.push_back(GraphicsApi::D3D11);
		if (isApiAvailable(GraphicsApi::D3D12)) apis.push_back(GraphicsApi::D3D12);
		return apis;
	}

	std::unique_ptr<IDevice> createDevice(GraphicsApi api)
	{
		switch (api)
		{
		case imp::gfx::GraphicsApi::Vulkan:
#if defined(IMP_GFX_VULKAN)
			return std::make_unique<vulkan::VulkanDevice>();
#else
			break;
#endif
		case imp::gfx::GraphicsApi::D3D12:
#if defined(IMP_GFX_DX12)
			return std::make_unique<dx12::DX12Device>();
#else
			break;
#endif
		case imp::gfx::GraphicsApi::D3D11:
#if defined(IMP_GFX_DX11)
			return std::make_unique<dx11::DX11Device>();
#else
			break;
#endif
		}
		LOG_ERROR("Graphics", "createDevice({}) requested but that backend wasn't compiled in for this build", toString(api));
		return nullptr;
	}

	const char* Version() { return "imp_gfx 0.1.0"; }
}