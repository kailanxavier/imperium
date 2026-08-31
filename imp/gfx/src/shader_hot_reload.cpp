#include <algorithm>
#include <gfx/shader_hot_reload.h>

#include <system_error>
#include <array>
#include <core/log/log.h>

namespace imp::gfx
{
    namespace
    {
        constexpr std::array<const char*, 3> kCompilableExtensions = {
            ".vert", ".frag", ".comp" };
        bool isCompilable(const std::filesystem::path& path)
        {
            const std::string ext = path.extension().string();
            for (const char* watched : kCompilableExtensions)
            {
                if (ext == watched)
                    return true;
            }
            return false;
        }
    }

    ShaderHotReloadWatcher::ShaderHotReloadWatcher(std::filesystem::path sourceRoot, std::filesystem::path spirvOutputDir, ShaderCompiler compiler)
        : m_sourceRoot(std::move(sourceRoot))
        , m_spirvOutputDir(std::move(spirvOutputDir))
        , m_compiler(std::move(compiler))
        , m_watcher(m_sourceRoot, std::vector<std::string>{ ".vert", ".frag", ".comp", ".glsl" })
    {
    }

    std::vector<std::filesystem::path> ShaderHotReloadWatcher::allCompilableSources() const
    {
        std::vector<std::filesystem::path> sources;

        std::error_code ec;
        auto it = std::filesystem::recursive_directory_iterator(
            m_sourceRoot, std::filesystem::directory_options::skip_permission_denied, ec);
        const auto end = std::filesystem::recursive_directory_iterator();

        for (; !ec && it != end; it.increment(ec))
        {
            const std::filesystem::directory_entry& entry = *it;

            std::error_code fileEc;
            if (!entry.is_regular_file(fileEc) || fileEc || !isCompilable(entry.path()))
                continue;

            std::filesystem::path relative = std::filesystem::relative(entry.path(), m_sourceRoot, fileEc);
            if (!fileEc)
                sources.push_back(std::move(relative));
        }

        return sources;
    }

    bool ShaderHotReloadWatcher::recompiled(const std::filesystem::path &relativeSource) const
    {
        const std::filesystem::path source = m_sourceRoot / relativeSource;
        const std::filesystem::path output = m_spirvOutputDir /
            (relativeSource.filename().string() + ".spv");

        std::string errorLog;
        if (m_compiler.compile(source, output, errorLog))
        {
            LOG_INFO("Shader", "Recompiled '{}'", relativeSource.generic_string().c_str());
            return true;
        }

        LOG_ERROR("Shader", "Hot reload of '{}' failed, keeping last working build:\n{}",
            relativeSource.generic_string().c_str(), errorLog.c_str());
        return false;
    }

    bool ShaderHotReloadWatcher::poll()
    {
        if (!isValid())
            return false;

        const std::vector<std::string> changed = m_watcher.poll();
        if (changed.empty())
            return false;

        const bool includeChanged = std::ranges::any_of(changed.begin(), changed.end(),
            [](const std::string& relative) { return std::filesystem::path(relative).extension() == ".glsl"; });

        std::vector<std::filesystem::path> targets;
        if (includeChanged)
        {
            targets = allCompilableSources();
        }
        else
        {
            for (const std::string& relative : changed)
            {
                std::filesystem::path path(relative);
                if (isCompilable(path))
                    targets.push_back(std::move(path));
            }
        }

        bool anySucceeded = false;
        for (const std::filesystem::path& target : targets)
            anySucceeded |= recompiled(target);

        return anySucceeded;
    }



}
