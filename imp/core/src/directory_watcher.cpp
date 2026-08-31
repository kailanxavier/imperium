#include <core/fs/directory_watcher.h>

#include <system_error>
#include <unordered_set>

#include "core/math/math.h"

namespace imp::fs
{
    DirectoryWatcher::DirectoryWatcher(std::filesystem::path root, std::vector<std::string> extensions)
        : m_root(std::move(root))
        , m_extensions(std::move(extensions))
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(m_root, ec) || ec)
            m_root.clear();
    }

    bool DirectoryWatcher::matchesExtension(const std::filesystem::path &path) const
    {
        if (m_extensions.empty())
            return true;

        const std::string ext = path.extension().string();
        for (const std::string& watched : m_extensions)
        {
            if (ext == watched)
                return true;
        }
        return false;
    }

    std::vector<std::string> DirectoryWatcher::poll()
    {
        std::vector<std::string> changed;
        if (!isValid())
            return changed;

        std::error_code ec;
        std::unordered_set<std::string> seenThisPoll;

        auto it = std::filesystem::recursive_directory_iterator(
            m_root, std::filesystem::directory_options::skip_permission_denied, ec);
        const auto end = std::filesystem::recursive_directory_iterator();

        for (; !ec && it != end; it.increment(ec))
        {
            const std::filesystem::directory_entry& entry = *it;

            std::error_code fileEc;
            if (!entry.is_regular_file(fileEc) || fileEc)
                continue;

            if (!matchesExtension(entry.path()))
                continue;

            const std::filesystem::path relative = std::filesystem::relative(entry.path(),
                m_root, fileEc);
            if (fileEc)
                continue;;

            const std::string relativeKey = relative.generic_string();
            const std::filesystem::file_time_type mtime = entry.last_write_time(fileEc);

            if (fileEc)
                continue;

            seenThisPoll.insert(relativeKey);

            const auto known = m_knownMTimes.find(relativeKey);
            if (known == m_knownMTimes.end())
            {
                m_knownMTimes.emplace(relativeKey, mtime);
                if (m_hasBaseline)
                    changed.push_back(relativeKey);
            }
            else if (known->second != mtime)
            {
                known->second = mtime;
                changed.push_back(relativeKey);
            }
        }

        for (auto entryIt = m_knownMTimes.begin(); entryIt != m_knownMTimes.end();)
        {
            if (!seenThisPoll.contains(entryIt->first))
                entryIt = m_knownMTimes.erase(entryIt);
            else
                ++entryIt;
        }

        m_hasBaseline = true;
        return changed;
    }



}
