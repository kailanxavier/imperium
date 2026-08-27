#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
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

		[[nodiscard]] bool isValid() const noexcept { return !m_physicalRoot.empty(); }
		std::vector<std::string> poll();

	private:
		std::string m_virtualPrefix;
		std::string m_extension;
		std::filesystem::path m_physicalRoot;

		std::unordered_map<std::string, std::filesystem::file_time_type> m_knownMTimes;
		bool m_hasBaseline = false;
	};
}
