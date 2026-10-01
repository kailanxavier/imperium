#pragma once

#include <core/fs/vfs.h>
#include <gfx/device.h>

namespace imp::jobs { class JobSystem; }
namespace imp::fwk { class LayerStack; }

namespace imp::engine
{
	struct RenderContext
	{
		gfx::IDevice& gfx;
		fs::VirtualFileSystem& vfs;
		jobs::JobSystem& jobs;

		fwk::LayerStack* layers = nullptr;
	};
}
