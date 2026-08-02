#pragma once
#include <core/math/math.h>
#include <core/types/int_types.h>
#include <ecs/entity.h>
#include <vector>

namespace imp::ecs
{
	enum class LightType : u8 { Directional, Point };
	class LightStorage
	{
	public:
		static constexpr u32 kInvalidDense = 0xFFFFFFFFU;
		LightStorage() = default;
		void create(EntityId entity, LightType type, const math::Vec3f& colour, float intensity);
		void destroy(EntityId entity);

		[[nodiscard]] bool contains(EntityId entity) const;
		void setType(EntityId entity, LightType type);
		void setColour(EntityId entity, const math::Vec3f& colour);
		void setIntensity(EntityId entity, float intensity);
		[[nodiscard]] LightType type(EntityId entity) const;
		[[nodiscard]] math::Vec3f colour(EntityId entity) const;
		[[nodiscard]] float intensity(EntityId entity) const;
		[[nodiscard]] size_t size() const noexcept { return m_owner.size(); }
		[[nodiscard]] const std::vector<EntityId>& owners() const noexcept { return m_owner; }
		[[nodiscard]] const std::vector<LightType>& types() const noexcept { return m_type; }
		[[nodiscard]] const std::vector<math::Vec3f>& colours() const noexcept { return m_colour; }
		[[nodiscard]] const std::vector<float>& intensities() const noexcept { return m_intensity; }
	private:

	private:
		u32 denseIndexOf(EntityId entity) const;
		void swapRemoveDense(u32 idx);
		std::vector<EntityId> m_owner;
		std::vector<LightType> m_type;
		std::vector<math::Vec3f> m_colour;
		std::vector<float> m_intensity;
		std::vector<u32> m_sparse;
	};
}
