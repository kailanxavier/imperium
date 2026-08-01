#include <ecs/light_storage.h>

namespace imp::ecs
{
	void LightStorage::create(EntityId entity, LightType type, const math::Vec3f& colour, float intensity)
	{
		assert(!contains(entity) && "LightStorage::create: entity already present");

		const u32 newDense = static_cast<u32>( m_owner.size() );
		m_owner.push_back(entity);
		m_type.push_back(type);
		m_colour.push_back(colour);
		m_intensity.push_back(intensity);

		if (entity.index >= m_sparse.size())
			m_sparse.resize(static_cast<size_t>( entity.index ) + 1, kInvalidDense);
		m_sparse[entity.index] = newDense;
	}

	void LightStorage::destroy(EntityId entity)
	{
		const u32 idx = denseIndexOf(entity);
		if (idx == kInvalidDense)
			return;

		swapRemoveDense(idx);
		m_sparse[entity.index] = kInvalidDense;
	}

	bool LightStorage::contains(EntityId entity) const
	{
		if (!entity.isValid() || entity.index >= m_sparse.size())
			return false;

		const u32 dense = m_sparse[entity.index];
		if (dense == kInvalidDense)
			return false;

		return m_owner[dense] == entity;
	}

	void LightStorage::setType(EntityId entity, LightType type)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::setType: entity not present");
		if (idx != kInvalidDense) m_type[idx] = type;
	}

	void LightStorage::setColour(EntityId entity, const math::Vec3f& colour)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::setColour: entity not present");
		if (idx != kInvalidDense) m_colour[idx] = colour;
	}

	void LightStorage::setIntensity(EntityId entity, float intensity)
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::setIntensity: entity not present");
		if (idx != kInvalidDense) m_intensity[idx] = intensity;
	}

	LightType LightStorage::type(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::type: entity not present");
		return ( idx == kInvalidDense ) ? LightType::Directional : m_type[idx];
	}

	math::Vec3f LightStorage::colour(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::colour: entity not present");
		return ( idx == kInvalidDense ) ? math::Vec3f::zero() : m_colour[idx];
	}

	float LightStorage::intensity(EntityId entity) const
	{
		const u32 idx = denseIndexOf(entity);
		assert(idx != kInvalidDense && "LightStorage::intensity: entity not present");
		return ( idx == kInvalidDense ) ? 0.f : m_intensity[idx];
	}

	u32 LightStorage::denseIndexOf(EntityId entity) const
	{
		if (!contains(entity))
			return kInvalidDense;

		return m_sparse[entity.index];
	}

	void LightStorage::swapRemoveDense(u32 idx)
	{
		const u32 last = static_cast<u32>( m_owner.size() ) - 1;
		if (idx != last)
		{
			m_owner[idx] = m_owner[last];
			m_type[idx] = m_type[last];
			m_colour[idx] = m_colour[last];
			m_intensity[idx] = m_intensity[last];
			m_sparse[m_owner[idx].index] = idx;
		}
		m_owner.pop_back();
		m_type.pop_back();
		m_colour.pop_back();
		m_intensity.pop_back();
	}

}
