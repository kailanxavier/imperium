#pragma once

#include <gfx/model.h>
#include <string>

namespace imp::fs { class VirtualFileSystem; }

namespace imp::gfx
{
	class IDevice;

	Model loadModel(IDevice& device, const std::string& path, const fs::VirtualFileSystem* vfs = nullptr);
}
