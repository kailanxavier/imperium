#pragma once

#include <core/memory/int_types.h>
#include <vector>
#include <string>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::gfx
{
	struct ImageData
	{
		u32 width = 0;
		u32 height = 0;
		std::vector<u8> pixels; // width * height * 4

		[[nodiscard]] bool isValid() const { return width > 0 && height > 0 && !pixels.empty(); }
	};

	[[nodiscard]] ImageData loadImageFromFile(const std::string& path, const fs::VirtualFileSystem* vfs = nullptr);
	[[nodiscard]] ImageData loadImageFromMemory(const std::vector<u8>& bytes);
}
