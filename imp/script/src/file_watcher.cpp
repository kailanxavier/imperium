#include <script/file_watcher.h>
#include <core/fs/vfs.h>

namespace imp::script
{
	namespace
	{
		std::string normalisePrefix(const std::string& prefix)
		{
			std::string normalised = fs::VirtualFileSystem::normalisePath(prefix);
			if (!normalised.empty() && normalised.back() != '/')
				normalised += '/';
			return normalised;
		}

		std::filesystem::path resolveMountPhysicalRoot(fs::VirtualFileSystem& vfs, const std::string& normalisedPrefix)
		{
			std::filesystem::path bestMatch;
			size_t longestMatch = 0;

			for (const auto& mount : vfs.mounts())
			{
				std::string mountPrefix = normalisePrefix(mount.virtualPrefix);

				if (normalisedPrefix.starts_with(mountPrefix))
				{
					if (mountPrefix.length() > longestMatch)
					{
						longestMatch = mountPrefix.length();
						std::string remainder = normalisedPrefix.substr(mountPrefix.length());
						bestMatch = std::filesystem::path(mount.physicalPath) / remainder;
					}
				}
			}

			return bestMatch;
		}
	}

	ScriptFileWatcher::ScriptFileWatcher(fs::VirtualFileSystem& vfs, std::string watchedVirtualPrefix, std::string extension)
		: m_virtualPrefix(normalisePrefix(watchedVirtualPrefix))
		, m_watcher(std::make_unique<fs::DirectoryWatcher>(resolveMountPhysicalRoot(vfs, m_virtualPrefix),
			std::vector<std::string>{std::move(extension)}))
	{ }

	std::vector<std::string> ScriptFileWatcher::poll() const
	{
		std::vector<std::string> changed;
		if (!isValid())
			return changed;

		for (std::string& relative : m_watcher->poll())
			changed.push_back(m_virtualPrefix + relative);
		return changed;
	}
}
