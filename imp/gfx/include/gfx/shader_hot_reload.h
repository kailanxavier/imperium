#pragma once

#include <filesystem>
#include <vector>

#include <core/fs/directory_watcher.h>
#include <gfx/shader_compiler.h>

namespace imp::gfx
{
    class ShaderHotReloadWatcher
    {
    public:
        ShaderHotReloadWatcher(std::filesystem::path sourceRoot,
            std::filesystem::path spirvOutputDir,
            ShaderCompiler compiler);

        ShaderHotReloadWatcher(const ShaderHotReloadWatcher&) = delete;
        ShaderHotReloadWatcher& operator=(const ShaderHotReloadWatcher&) = delete;

        [[nodiscard]] bool isValid() const noexcept { return m_watcher.isValid() && m_compiler.isValid(); }
        bool poll();

    private:
        std::vector<std::filesystem::path> allCompilableSources() const;
        bool recompiled(const std::filesystem::path& relativeSource) const;

        std::filesystem::path m_sourceRoot;
        std::filesystem::path m_spirvOutputDir;
        ShaderCompiler m_compiler;
        fs::DirectoryWatcher m_watcher;
    };
}
