#include <engine/engine_assets.h>

#include <core/log/log.h>
#include <core/platform/exe_path.h>
#include <gfx/shader_compiler.h>
#include <gfx/shader_hot_reload.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace imp::engine
{
	bool mountEngineAssets(fs::VirtualFileSystem& vfs)
	{
		const std::string prefix = fs::VirtualFileSystem::normalisePath(kEngineVfsPrefix);

		const auto& mounts = vfs.mounts();
		const bool alreadyMounted = std::ranges::any_of(mounts,
			[&](const fs::MountPoint& mount) { return mount.virtualPrefix == prefix; });
		if (alreadyMounted)
			return true;

		const std::string physical = ( platform::executableDir() / "engine" ).string();
		if (!vfs.mount(kEngineVfsPrefix, physical, 0, true, true))
		{
			LOG_ERROR("Engine", "Failed to mount {} -> {}", kEngineVfsPrefix, physical);
			return false;
		}

		return true;
	}

	std::unique_ptr<gfx::ShaderHotReloadWatcher> createShaderHotReloadWatcher(fs::VirtualFileSystem& vfs)
	{
#if defined(IMP_HOT_RELOAD_ENABLED)
		gfx::ShaderCompiler compiler(IMP_SHADER_COMPILER_PATH, IMP_SHADER_COMPILER_IS_GLSLANG_VALIDATOR != 0);
		const std::string outputDir = vfs.resolvePhysicalPath(kEngineShaderVfsDir, true);

		auto watcher = std::make_unique<gfx::ShaderHotReloadWatcher>(
			std::filesystem::path(IMP_SHADER_SOURCE_DIR), std::filesystem::path(outputDir), std::move(compiler));

		LOG_INFO("Engine", "Shader hot reload {}", watcher->isValid() ? "active" : "inactive (source dir or compiler not found");
		return watcher;
#else
		(void)vfs;
		return nullptr;
#endif
	}
}
