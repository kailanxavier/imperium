#pragma once

#include <fwk/window.h>
#include <fwk/input.h>
#include <fwk/layer.h>
#include <gfx/device.h>
#include <core/fs/vfs.h>

namespace imp::app
{
	struct AppContext
	{
		fwk::Window& window;
		gfx::IDevice& gfx;
		fwk::Input& input;
		fwk::LayerStack& layers;
		fs::VirtualFileSystem& vfs;
	};

	class IApp
	{
	public:
		virtual ~IApp() = default;
		virtual bool onInit(AppContext& ctx) = 0;
		virtual void onUpdate(AppContext& ctx, float deltaSeconds) = 0;
		virtual void onRender(AppContext& ctx, gfx::ICommandList& cmd) = 0;
		virtual void onShutdown(AppContext& ctx) = 0;
	};
}
