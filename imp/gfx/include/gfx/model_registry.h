#pragma once
#include <gfx/model.h>
#include <gfx/model_handle.h>

#include <core/types/int_types.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::jobs { class JobSystem; }

namespace imp::gfx
{
	class IDevice;
	class TextureCache;

	class ModelRegistry
	{
	public:
		ModelRegistry() = default;
		~ModelRegistry();

		ModelHandle load(IDevice& device, const std::string& path, jobs::JobSystem& jobSystem, 
			const fs::VirtualFileSystem* vfs = nullptr);

		[[nodiscard]] bool isValid(ModelHandle handle) const;
		[[nodiscard]] Model* tryGet(ModelHandle handle);
		[[nodiscard]] const Model* tryGet(ModelHandle handle) const;

		void unload(ModelHandle handle);
		void clear();
		void shutdown();

		[[nodiscard]] size_t residentCount() const noexcept { return m_slots.size() - m_freeList.size(); }

	private:
		struct Slot
		{
			Model model;
			std::string path;
			u32 generation = 0;
			bool occupied;
		};

		std::vector<Slot> m_slots;
		std::unordered_map<std::string, u32> m_pathToIndex;
		std::vector<u32> m_freeList;
		std::unique_ptr<TextureCache> m_textureCache;
	};
}
