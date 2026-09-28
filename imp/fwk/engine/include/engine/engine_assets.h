#pragma once

#include <core/fs/vfs.h>
#include <memory>

namespace imp::gfx
{
	class ShaderHotReloadWatcher;
}

namespace imp::engine
{
	inline constexpr const char* kEngineVfsPrefix = "engine/";
	inline constexpr const char* kEngineShaderVfsDir = "engine/shaders/";
	bool mountEngineAssets(fs::VirtualFileSystem& vfs);
	std::unique_ptr<gfx::ShaderHotReloadWatcher> createShaderHotReloadWatcher(fs::VirtualFileSystem& vfs);
}
