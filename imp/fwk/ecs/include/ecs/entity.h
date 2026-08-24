#pragma once

#include <core/types/int_types.h>
#include <vector>
#include <cstddef>
#include <functional>

namespace imp::ecs 
{
	struct EntityId
	{
		static constexpr u32 kInvalidIndex = 0xFFFFFFFFU;

		u32 index = kInvalidIndex;
		u32 generation = 0;

		constexpr bool operator==(const EntityId& other) const noexcept
		{
			return index == other.index && generation == other.generation;
		}

		constexpr bool operator!=(const EntityId& other) const noexcept
		{
			return !( *this == other );
		}

		constexpr bool isValid() const noexcept
		{
			return index != kInvalidIndex;
		}
	};

	inline constexpr EntityId kInvalidEntity{};

	class EntityRegistry
	{
	public:
		EntityRegistry() = default;

		EntityId create();
		void destroy(EntityId entity);
		bool isAlive(EntityId entity) const noexcept;

		[[nodiscard]] size_t aliveCount() const noexcept { return m_aliveCount; }
		[[nodiscard]] size_t capacity() const noexcept { return m_generations.size(); }

	private:
		std::vector<u32> m_generations;
		std::vector<u32> m_freeList;
		size_t m_aliveCount = 0;
	};
}

namespace std
{
	template<>
	struct hash<imp::ecs::EntityId>
	{
		size_t operator()(const imp::ecs::EntityId& id) const noexcept
		{
			return ( static_cast<size_t>( id.index ) << 32 ) ^ static_cast<size_t>( id.generation );
		}
	};
}
