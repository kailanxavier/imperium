#pragma once

#include <core/fs/directory_watcher.h>

#include <string>
#include <memory>
#include <vector>

namespace imp::fs { class VirtualFileSystem; }

namespace imp::script
{
	class ScriptFileWatcher
	{
	public:
		ScriptFileWatcher(fs::VirtualFileSystem& vfs, 
			std::string watchedVirtualPrefix, std::string extension = ".lua");

		ScriptFileWatcher(const ScriptFileWatcher&) = delete;
		ScriptFileWatcher& operator=(const ScriptFileWatcher&) = delete;

		[[nodiscard]] bool isValid() const noexcept { return m_watcher && m_watcher->isValid(); }
		[[nodiscard]] std::vector<std::string> poll() const;

	private:
		std::string m_virtualPrefix;
		std::unique_ptr<fs::DirectoryWatcher> m_watcher;
	};
}
