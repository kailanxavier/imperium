#include <gfx/image.h>

#include <core/fs/vfs.h>
#include <core/log/log.h>

#include <stb_image.h>
#include <fstream>

namespace imp::gfx
{
	namespace
	{
		bool readFileBytes(const std::string& path, const fs::VirtualFileSystem* vfs, std::vector<u8>& outBytes)
		{
			if (vfs)
				return vfs->readEntireFile(path, outBytes);

			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file.is_open())
				return false;

			std::streamsize size = file.tellg();
			if (size <= 0)
				return false;

			file.seekg(0, std::ios::beg);
			outBytes.resize(static_cast<size_t>( size ));
			return static_cast<bool>( file.read(reinterpret_cast<char*>( outBytes.data() ), size) );
		}
	}

	ImageData loadImageFromFile(const std::string& path, const fs::VirtualFileSystem* vfs)
	{
		std::vector<u8> bytes;
		if (!readFileBytes(path, vfs, bytes))
		{
			LOG_ERROR("Image Loader", "loadImageFromFile failed to read: {}", path.c_str());
			return {};
		}

		return loadImageFromMemory(bytes);
	}

	ImageData loadImageFromMemory(const std::vector<u8>& bytes)
	{
		ImageData result;

		int width = 0, height = 0, channelsInFile = 0;
		stbi_uc* decoded = stbi_load_from_memory(
			bytes.data(), static_cast<int>( bytes.size() ), &width, &height, &channelsInFile, STBI_rgb_alpha
		);

		if (!decoded)
		{
			LOG_ERROR("Image Loader", "stbi_load_from_memory failed: {}", stbi_failure_reason());
			return result;
		}

		result.width = static_cast<u32>( width );
		result.height = static_cast<u32>( height );

		const size_t pixelCount = 
			static_cast<size_t>( width ) * 
			static_cast<size_t>( height ) * 
			static_cast<int>( STBI_rgb_alpha );

		result.pixels.assign(decoded, decoded + pixelCount);

		stbi_image_free(decoded);
		return result;
	}
}
