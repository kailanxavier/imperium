#include <gfx/model_registry.h>
#include <gfx/model_loader.h>
#include <core/log/log.h>

#include <utility>

namespace imp::gfx
{
	ModelHandle ModelRegistry::load(IDevice& device, const std::string& path, const fs::VirtualFileSystem* vfs)
	{
		if (const auto it = m_pathToIndex.find(path); it != m_pathToIndex.end())
		{
			const u32 idx = it->second;
			return ModelHandle{ idx, m_slots[idx].generation };
		}

		Model model = loadModel(device, path, vfs);
		if (!model.isValid())
		{
			LOG_ERROR("Model Registry", "Failed to load model {}", path.c_str());
			return ModelHandle{};
		}

		u32 idx;
		if (!m_freeList.empty())
		{
			idx = m_freeList.back();
			m_freeList.pop_back();
		}
		else
		{
			idx = static_cast<u32>( m_slots.size() );
			m_slots.emplace_back();
		}

		Slot& slot = m_slots[idx];
		slot.model = std::move(model);
		slot.path = path;
		slot.occupied = true;

		m_pathToIndex.emplace(path, idx);
		return ModelHandle{ idx, slot.generation };
	}

	bool ModelRegistry::isValid(ModelHandle handle) const
	{
		if (!handle.isValid() || handle.index >= m_slots.size())
			return false;

		const Slot& slot = m_slots[handle.index];
		return slot.occupied && slot.generation == handle.generation;
	}

	Model* ModelRegistry::tryGet(ModelHandle handle)
	{
		return isValid(handle) ? &m_slots[handle.index].model : nullptr;
	}

	const Model* ModelRegistry::tryGet(ModelHandle handle) const
	{
		return isValid(handle) ? &m_slots[handle.index].model : nullptr;
	}

	void ModelRegistry::unload(ModelHandle handle)
	{
		if (!isValid(handle))
			return;

		Slot& slot = m_slots[handle.index];
		m_pathToIndex.erase(slot.path);
		slot.model = Model{};
		slot.path.clear();
		slot.occupied = false;
		++slot.generation;

		m_freeList.push_back(handle.index);
	}

	void ModelRegistry::clear()
	{
		m_slots.clear();
		m_pathToIndex.clear();
		m_freeList.clear();
	}
}
