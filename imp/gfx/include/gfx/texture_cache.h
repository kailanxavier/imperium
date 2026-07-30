#pragma once

#include <core/memory/int_types.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace imp::gfx
{
	class IDevice;
	class ITexture;

	class TextureCache
	{
	public:
		explicit TextureCache(IDevice& device) : m_device(device) {}

		TextureCache(const TextureCache&) = delete;
		TextureCache& operator=(const TextureCache&) = delete;

		[[nodiscard]] std::shared_ptr<ITexture> find(const std::string& cacheKey);

		std::shared_ptr<ITexture> insert(const std::string& cacheKey, std::shared_ptr<ITexture> texture);

		[[nodiscard]] std::shared_ptr<ITexture> fallbackAlbedo();
		[[nodiscard]] std::shared_ptr<ITexture> fallbackMetallicRoughness();
		[[nodiscard]] std::shared_ptr<ITexture> fallbackNormal();
		[[nodiscard]] std::shared_ptr<ITexture> fallbackOcclusion();

	private:
		std::shared_ptr<ITexture> getOrCreateFallback(std::shared_ptr<ITexture>& slot, u8 r, u8 g, u8 b, u8 a);

		IDevice& m_device;

		std::mutex m_pathMutex;
		std::unordered_map<std::string, std::shared_ptr<ITexture>> m_pathTextures;

		std::mutex m_fallbackMutex;
		std::shared_ptr<ITexture> m_fallbackAlbedo;
		std::shared_ptr<ITexture> m_fallbackMetallicRoughness;
		std::shared_ptr<ITexture> m_fallbackNormal;
		std::shared_ptr<ITexture> m_fallbackOcclusion;
	};
}
