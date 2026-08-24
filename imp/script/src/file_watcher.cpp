#include <script/file_watcher.h>

#include <core/fs/vfs.h>

#include <system_error>
#include <unordered_set>

namespace imp::script
{
	namespace
	{
		std::filesystem::path resolveMountPhysicalRoot(fs::VirtualFileSystem& vfs, const std::string& normalisedPrefix)
		{
			for (const fs::MountPoint& mount : vfs.mounts())
			{
				if (mount.virtualPrefix == normalisedPrefix)
					return std::filesystem::path(mount.physicalPath);
			}

			return {};
		}

		std::string normalisePrefix(const std::string& prefix)
		{
			std::string normalised = fs::VirtualFileSystem::normalisePath(prefix);
			if (!normalised.empty() && normalised.back() != '/')
				normalised += '/';
			return normalised;
		}
	}

	ScriptFileWatcher::ScriptFileWatcher(fs::VirtualFileSystem& vfs, std::string watchedVirtualPrefix, std::string extension)
		: m_virtualPrefix(normalisePrefix(watchedVirtualPrefix))
		, m_extension(std::move(extension))
		, m_physicalRoot(resolveMountPhysicalRoot(vfs, m_virtualPrefix))
	{ }

	std::vector<std::string> ScriptFileWatcher::poll()
	{
		std::vector<std::string> changed;
		if (!isValid())
			return changed;

		std::error_code ec;
		std::unordered_set<std::string> seenThisPoll;

		auto it = std::filesystem::recursive_directory_iterator(
			m_physicalRoot, std::filesystem::directory_options::skip_permission_denied, ec);
		const auto end = std::filesystem::recursive_directory_iterator();

		for (; !ec && it != end; it.increment(ec))
		{
			const std::filesystem::directory_entry& entry = *it;

			std::error_code fileEc;
			if (!entry.is_regular_file(fileEc) || fileEc)
				continue;

			if (entry.path().extension() != m_extension)
				continue;

			const std::filesystem::path relative = std::filesystem::relative(entry.path(), m_physicalRoot, fileEc);
			if (fileEc)
				continue;

			const std::string virtualPath = m_virtualPrefix + relative.generic_string();

			const std::filesystem::file_time_type mtime = entry.last_write_time(fileEc);
			if (fileEc)
				continue;

			seenThisPoll.insert(virtualPath);

			const auto known = m_knownMTimes.find(virtualPath);
			if (known == m_knownMTimes.end())
			{
				m_knownMTimes.emplace(virtualPath, mtime);
				if (m_hasBaseline)
					changed.push_back(virtualPath);
			}
			else if (known->second != mtime)
			{
				known->second = mtime;
				changed.push_back(virtualPath);
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
