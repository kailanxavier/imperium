#include <gfx/image.h>

#include <core/fs/vfs.h>
#include <core/log/log.h>

#include <turbojpeg.h>
#include <spng.h>
#include <libdeflate.h>

#include <stb_image.h>
#include <fstream>

#include <cstdio>
#include <cmath>

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

		bool isJpeg(const u8* data, size_t size)
		{
			return size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
		}

		bool isPng(const u8* data, size_t size)
		{
			static constexpr u8 kPngMagic[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
			return size >= 8 && std::memcmp(data, kPngMagic, 8) == 0;
		}

		ImageData decodeWithTurboJpeg(const u8* data, size_t size)
		{
			ImageData result;

			tjhandle handle = tjInitDecompress();
			if (!handle)
			{
				LOG_ERROR("Image Loader", "tjInitDecompress failed");
				return result;
			}

			int width = 0, height = 0, subsamp = 0, colorspace = 0;
			if (tjDecompressHeader3(handle, data, static_cast<unsigned long>( size ),
				&width, &height, &subsamp, &colorspace) != 0)
			{
				LOG_ERROR("Image Loader", "tjDecompressHeader3 failed: {}", tjGetErrorStr2(handle));
				tjDestroy(handle);
				return result;
			}

			const size_t pixelCount = static_cast<size_t>( width ) * static_cast<size_t>( height ) * 4;
			result.pixels.resize(pixelCount);

			if (tjDecompress2(handle, data, static_cast<unsigned long>( size ),
				result.pixels.data(), width, /*pitch=*/0, height, TJPF_RGBA, TJFLAG_FASTDCT) != 0)
			{
				LOG_ERROR("Image Loader", "tjDecompress2 failed: {}", tjGetErrorStr2(handle));
				tjDestroy(handle);
				return {};
			}

			tjDestroy(handle);

			result.width = static_cast<u32>( width );
			result.height = static_cast<u32>( height );
			return result;
		}

		size_t libdeflateDecompress(void* ctx, spng_ctx*, const void* in, size_t inSize, void* out, size_t outSize)
		{
			auto* decompressor = static_cast<libdeflate_decompressor*>( ctx );
			size_t actualOut = 0;
			libdeflate_result result = libdeflate_deflate_decompress(
				decompressor, in, inSize, out, outSize, &actualOut);
			return result == LIBDEFLATE_SUCCESS ? actualOut : 0;
		}

		ImageData decodeWithSpng(const u8* data, size_t size)
		{
			ImageData result;

			spng_ctx* ctx = spng_ctx_new(0);
			if (!ctx)
			{
				LOG_ERROR("Image Loader", "spng_ctx_new failed");
				return result;
			}

			spng_set_png_buffer(ctx, data, size);

			spng_ihdr ihdr{};
			if (spng_get_ihdr(ctx, &ihdr) != 0)
			{
				LOG_ERROR("Image Loader", "spng_get_ihdr failed");
				spng_ctx_free(ctx);
				return result;
			}

			size_t outSize = 0;
			if (spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &outSize) != 0)
			{
				LOG_ERROR("Image Loader", "spng_decoded_image_size failed");
				spng_ctx_free(ctx);
				return result;
			}

			result.pixels.resize(outSize);
			if (spng_decode_image(ctx, result.pixels.data(), outSize, SPNG_FMT_RGBA8, 0) != 0)
			{
				LOG_ERROR("Image Loader", "spng_decode_image failed");
				spng_ctx_free(ctx);
				return {};
			}

			spng_ctx_free(ctx);

			result.width = ihdr.width;
			result.height = ihdr.height;
			return result;
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
		if (isJpeg(bytes.data(), bytes.size()))
		{
			ImageData result = decodeWithTurboJpeg(bytes.data(), bytes.size());
			if (result.isValid())
				return result;

			LOG_WARN("Image Loader", "TurboJPEG decode failed, falling back to stb_image");
		}
		else if (isPng(bytes.data(), bytes.size()))
		{
			ImageData result = decodeWithSpng(bytes.data(), bytes.size());
			if (result.isValid())
				return result;

			LOG_WARN("Image Loader", "spng decode failed, falling back to stb_image");
		}

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

	bool saveImageToFile(const ImageData& image, const std::string& path)
	{
		if (!image.isValid())
		{
			LOG_ERROR("Image Write", "saveImageToFile(): invalid image data");
			return false;
		}

		FILE* file = std::fopen(path.c_str(), "wb");
		if (!file)
		{
			LOG_ERROR("Image Writer", "saveImageToFile(): failed to open '{}' for writing", path.c_str());
			return false;
		}

		spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);
		if (!ctx)
		{
			LOG_ERROR("Image Writer", "Encoder failed");
			std::fclose(file);
			return false;
		}

		spng_set_png_file(ctx, file);

		spng_ihdr ihdr{};
		ihdr.width = image.width;
		ihdr.height = image.height;
		ihdr.bit_depth = 8;
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
		spng_set_ihdr(ctx, &ihdr);

		const int err = spng_encode_image(ctx, image.pixels.data(), image.pixels.size(),
			SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

		if (err != 0)
			LOG_ERROR("Image Writer", "spng_encode_image failed: {}", spng_strerror(err));

		spng_ctx_free(ctx);
		std::fclose(file);

		return err == 0;
	}

	ImageDiffResult compareImages(const ImageData& a, const ImageData& b, u8 perPixelThreshold)
	{
		ImageDiffResult result{};

		if (a.width != b.width || a.height != b.height)
		{
			result.sameSize = false;
			return result;
		}
		result.sameSize = true;

		if (a.pixels.size() != b.pixels.size())
			return result;

		u64 totalAbsError = 0;
		const size_t count = a.pixels.size();

		for (size_t i = 0; i < count; ++i)
		{
			const int diff = std::abs(static_cast<int>(a.pixels[i]) - static_cast<int>(b.pixels[i]));
			totalAbsError += static_cast<u64>(diff);
			result.maxAbsError = std::max(result.maxAbsError, static_cast<u32>(diff));

			if (i % 4 == 0)
			{
				bool pixelDiffers = false;
				for (u32 c = 0; c < 4; c++) // no way, he said the thing
				{
					if (std::abs(static_cast<int>(a.pixels[i + c]) - static_cast<int>(b.pixels[i + c])) > perPixelThreshold)
					{
						pixelDiffers = true;
						break;
					}
				}
				if (pixelDiffers)
					++result.diffPixelCount;
			}
		}

		result.meanAbsError = count > 0 ? static_cast<double>(totalAbsError) / static_cast<double>(count) : 0.0;
		return result;
	}


}
