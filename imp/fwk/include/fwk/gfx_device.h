#pragma once

#include <core/memory/int_types.h>
#include <core/memory/iallocator.h>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::fwk
{
	class Window;

	enum class GfxApi
	{
		Vulkan,
		D3D12,
		D3D11,
		/*Metal (absolutely not)*/
	};

	struct GfxDeviceDesc
	{
		Window* window = nullptr;
		const char* appName = "App";
		bool enableValidation = false;
		bool vsync = true;
		const fs::VirtualFileSystem* vfs = nullptr;
		memory::IAllocator* allocator = nullptr;
	};

	class IGfxDevice
	{
	public:
		virtual ~IGfxDevice() = default;

		virtual bool initialise(const GfxDeviceDesc& desc) = 0;
		virtual void shutdown() = 0;

		virtual void onResize(u32 width, u32 height) = 0;

		virtual void onMinimiseChanged(bool minimised)
		{
			(void)minimised; // default no-op; backends override if needed
		}

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual GfxApi getApi() const = 0;
		virtual const char* getApiName() const = 0;
	};
}