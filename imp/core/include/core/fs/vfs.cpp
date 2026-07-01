#include "vfs.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

namespace stdfs = std::filesystem;

namespace imp::fs
{
	std::string VirtualFileSystem::resolvePhysicalPath(const Path& virtualPath, bool forWrite) const
	{
		std::string norm = normalisePath(virtualPath);
		auto candidates = matchingMounts(norm);

		for (const MountPoint* mp : candidates)
		{
			if (forWrite && !mp->writable) continue;

			std::string remainder = norm.substr(mp->virtualPrefix.size());
			stdfs::path physical = stdfs::path(mp->physicalPath) / remainder;

			if (forWrite)
			{
				// Firt writable mount wins immediately, we don't require
				// the file to exist there already
				return physical.string();
			}

			std::error_code ec;
			if (stdfs::exists(physical, ec))
				return physical.string();
		}

		// Punish the caller for calling with bad paths?
		return "";
	}

	std::string VirtualFileSystem::normalisePath(const std::string& p)
	{
		std::string out;
		out.reserve(p.size());

		// Convert backslashes to single forward slash
		for (char c : p)
			out += ( c == '\\' ) ? '/' : c;

		// Collapse "./" and "//" segments, strip leading "/".
		std::vector<std::string> parts;
		size_t start = 0;
		while (start <= out.size())
		{
			size_t end = out.find('/', start);
			if (end == std::string::npos) end = out.size();
			std::string segment = out.substr(start, end - start);

			if (!segment.empty() && segment != ".")
				(segment == ".." && !parts.empty() && parts.back() != "..") ? parts.pop_back() : parts.push_back(segment);

			start = end + 1;
		}

		bool hadTrailingSlash = !out.empty() && out.back() == '/';

		std::string result;
		for (size_t i = 0; i < parts.size(); ++i)
		{
			if (i > 0) result += '/';
			result += parts[i];
		}

		if (hadTrailingSlash && !result.empty()) result += '/';
		return result;
	}

	void VirtualFileSystem::sortMountsByPriority()
	{
		std::stable_sort(m_mounts.begin(), m_mounts.end(),
			[](const MountPoint& a, const MountPoint& b)
			{
				return a.priority > b.priority;
			});
	}

	std::vector<const MountPoint*> VirtualFileSystem::matchingMounts(const std::string& normalisedVirtualPath) const
	{
		std::vector<const MountPoint*> result;
		for (const auto& mp : m_mounts)
		{
			if (normalisedVirtualPath.size() >= mp.virtualPrefix.size() &&
				normalisedVirtualPath.compare(0, mp.virtualPrefix.size(), mp.virtualPrefix) == 0)
			{
				result.push_back(&mp);
			}
		}

		// m_mounts is already sorted by priority descending, 
		// so 'result' preserves that order
		return result;
	}

	bool VirtualFileSystem::mount(const std::string& virtualPrefix, const std::string& physicalPath, int priority, bool writable, bool createIfMissing)
	{
		std::string normVirtual = normalisePath(virtualPrefix);
		if (!normVirtual.empty() && normVirtual.back() != '/') normVirtual += '/';

		stdfs::path physPath(physicalPath);
		std::error_code ec;

		if (!stdfs::exists(physPath, ec))
		{
			if (createIfMissing)
			{
				if (!stdfs::create_directories(physPath, ec))
					return false;
			}
			else
			{
				return false;
			}
		}
		else if (!stdfs::is_directory(physPath, ec))
		{
			return false; // physical path must be a directory
		}

		auto normPhysical = stdfs::absolute(physPath, ec).lexically_normal().string();

		MountPoint mp;
		mp.virtualPrefix = normVirtual;
		mp.physicalPath = normPhysical;
		mp.priority = priority;
		mp.writable = writable;

		m_mounts.push_back(std::move(mp));
		sortMountsByPriority();
		return true;
	}

	size_t VirtualFileSystem::unmount(const std::string& virtualPrefix, const std::string& physicalPath)
	{
		std::string normVirtual = normalisePath(virtualPrefix);
		if (!normVirtual.empty() && normVirtual.back() != '/') normVirtual += '/';

		std::error_code ec;
		std::string normPhysical;

		if (!physicalPath.empty())
			normPhysical = stdfs::absolute(stdfs::path(physicalPath), ec).lexically_normal().string();
		
		size_t before = m_mounts.size();
		m_mounts.erase(
			std::remove_if(m_mounts.begin(), m_mounts.end(),
				[&](const MountPoint& mp)
				{
					if (mp.virtualPrefix != normVirtual) return false;
					if (!normPhysical.empty() && mp.physicalPath != normPhysical) return false;
					return true;
				}),
			m_mounts.end());

		return before - m_mounts.size();
	}

	void VirtualFileSystem::unmountAll()
	{
		m_mounts.clear();
	}

	bool VirtualFileSystem::exists(const Path& virtualPath) const
	{
		return !resolvePhysicalPath(virtualPath, false).empty();
	}

	bool VirtualFileSystem::isDirectory(const Path& virtualPath) const
	{
		auto phys = resolvePhysicalPath(virtualPath, false);
		if (phys.empty()) return false;

		std::error_code ec;
		return stdfs::is_directory(phys, ec);
	}

	bool VirtualFileSystem::readEntireFile(const Path& virtualPath, Bytes& outData) const
	{
		auto phys = resolvePhysicalPath(virtualPath, false);
		if (phys.empty()) return false;

		std::ifstream file(phys, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return false;

		std::streamsize size = file.tellg();
		if (size < 0) return false;
		file.seekg(0, std::ios::beg);

		outData.resize(static_cast<size_t>(size));

		if (size > 0 && !file.read(reinterpret_cast<char*>( outData.data() ), size))
			return false;
		
		return true;
	}

	bool VirtualFileSystem::readEntireFileText(const Path& virtualPath, std::string& outText) const
	{
		Bytes data;
		if (!readEntireFile(virtualPath, data)) return false;
		outText.assign(reinterpret_cast<const char*>( data.data() ), data.size());
		return true;
	}

	bool VirtualFileSystem::writeEntireFile(const Path& virtualPath, const Bytes& data, bool createDirs) const
	{
		auto phys = resolvePhysicalPath(virtualPath, true);
		if (phys.empty()) return false;

		if (createDirs)
		{
			std::error_code ec;
			stdfs::path parent = stdfs::path(phys).parent_path();
			if (!parent.empty()) stdfs::create_directories(parent, ec);
		}
		
		std::ofstream file(phys, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) return false;

		if (!data.empty())
		{
			file.write(
				reinterpret_cast<const char*>( data.data() ), 
				static_cast<std::streamsize>(data.size()));
		}

		return file.good();
	}

	bool VirtualFileSystem::writeEntireFileText(const Path& virtualPath, const std::string& text, bool createDirs) const
	{
		Bytes data(text.begin(), text.end());
		return writeEntireFile(virtualPath, data, createDirs);
	}

	bool VirtualFileSystem::Delete(const Path& virtualPath) const
	{
		auto phys = resolvePhysicalPath(virtualPath, false);
		if (phys.empty()) return false;

		std::error_code ec;
		return stdfs::remove(phys, ec);
	}

	bool VirtualFileSystem::createDirectory(const Path& virtualDir) const
	{
		auto norm = normalisePath(virtualDir);
		if (!norm.empty() && norm.back() != '/') norm += '/';

		auto candidates = matchingMounts(norm);
		for (const MountPoint* mp : candidates)
		{
			if (!mp->writable) continue;
			std::string remainder = norm.substr(mp->virtualPrefix.size());
			stdfs::path physical = stdfs::path(mp->physicalPath) / remainder;
			std::error_code ec;

			return stdfs::create_directories(physical, ec) || stdfs::is_directory(physical, ec);
		}
		return false;
	}

	std::vector<std::string> VirtualFileSystem::listFiles(const Path& virtualDir, bool recursive) const
	{
		std::string norm = normalisePath(virtualDir);
		if (!norm.empty() && norm.back() != '/') norm += '/';

		auto candidates = matchingMounts(norm);
		std::set<std::string> seen;
		std::vector<std::string> results;

		for (const MountPoint* mp : candidates)
		{
			std::string remainder = norm.substr(mp->virtualPrefix.size());
			stdfs::path physicalDir = stdfs::path(mp->physicalPath) / remainder;

			std::error_code ec;
			if (!stdfs::is_directory(physicalDir, ec)) continue;

			auto visit = [&](const stdfs::directory_entry& entry)
				{
					if (!entry.is_regular_file(ec)) return;
					stdfs::path rel = stdfs::relative(entry.path(), mp->physicalPath, ec);
					std::string virtualFile = mp->virtualPrefix + rel.generic_string();

					if (seen.insert(virtualFile).second)
						results.push_back(virtualFile);
				};

			if (recursive)
			{
				for (const auto& entry : stdfs::recursive_directory_iterator(physicalDir, ec))
					visit(entry);
			}
			else
			{
				for (const auto& entry : stdfs::directory_iterator(physicalDir, ec))
					visit(entry);
			}
		}

		return results;
	}
}