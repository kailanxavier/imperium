#pragma once

#include <gfx/model.h>
#include <string>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::jobs { class JobSystem; }

namespace imp::gfx
{
	class IDevice;
	class TextureCache;

	Model loadModel(IDevice& device, const std::string& path, 
		jobs::JobSystem& jobSystem, TextureCache& textureCache, 
		const fs::VirtualFileSystem* vfs = nullptr);
}
