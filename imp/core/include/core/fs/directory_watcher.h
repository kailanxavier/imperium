#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace imp::fs
{
    class DirectoryWatcher
    {
    public:
        DirectoryWatcher(std::filesystem::path root, std::vector<std::string> extensions);

        DirectoryWatcher(const DirectoryWatcher&) = delete;
        DirectoryWatcher& operator=(const DirectoryWatcher&) = delete;

        [[nodiscard]] bool isValid() const noexcept { return !m_root.empty(); }
        [[nodiscard]] const std::filesystem::path& root() const noexcept { return m_root; }

        std::vector<std::string> poll();

    private:
        [[nodiscard]] bool matchesExtension(const std::filesystem::path& path) const;

        std::filesystem::path m_root;
        std::vector<std::string> m_extensions;

        std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> m_knownMTimes;

        bool m_hasBaseline = false;
    };
}
