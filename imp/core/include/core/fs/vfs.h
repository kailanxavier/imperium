#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace imp::fs
{
	using Path = std::string;
	using Bytes = std::vector<uint8_t>;

	struct MountPoint
	{
		std::string virtualPrefix;
		std::string physicalPath;
		int priority = 0;
		bool writable = true;
	};

	class VirtualFileSystem
	{
	public:
		VirtualFileSystem() = default;
		bool mount(const std::string& virtualPrefix,
			const std::string& physicalPath,
			int priority = 0,
			bool writable = true,
			bool createIfMissing = false);

		size_t unmount(const std::string& virtualPrefix, const std::string& physicalPath = "");

		void unmountAll();

		const std::vector<MountPoint>& mounts() const { return m_mounts; }

		// Queries
		bool exists(const Path& virtualPath) const;
		bool isDirectory(const Path& virtualPath) const;

		bool readEntireFile(const Path& virtualPath, Bytes& outData) const;
		bool readEntireFileText(const Path& virtualPath, std::string& outText) const;

		bool writeEntireFile(const Path& virtualPath, const Bytes& data, bool createDirs = true) const;
		bool writeEntireFileText(const Path& virtualPath, const std::string& text, bool createDirs = true) const;

		bool Delete(const Path& virtualPath) const;
		bool createDirectory(const Path& virtualDir) const;

		std::vector<std::string> listFiles(const Path& virtualDir, bool recursive = false) const;

		std::string resolvePhysicalPath(const Path& virtualPath, bool forWrite = false) const;
		static std::string normalisePath(const std::string& p);

	private:
		std::vector<MountPoint> m_mounts;
		void sortMountsByPriority();

		std::vector<const MountPoint*> matchingMounts(const std::string& normalisedVirtualPath) const;
	};
}