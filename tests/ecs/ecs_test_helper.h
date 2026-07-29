#include <ecs/entity.h>
#include <ecs/transform_storage.h>
#include <jobs/job_system.h>

#include <vector>

inline static constexpr float kEpsF = 1e-5f;
inline static constexpr float kEpsFF = 1e-4f; 
inline static constexpr double kEpsD = 1e-10f;

using namespace imp::ecs;
using namespace imp::math;
using namespace imp::jobs;

inline static Transform makeTestTransform(u32 order)
{
	Transform t;
	t.position = Vec3f
	{
		static_cast<float>( order % 13 ) * 0.37f - 2.4f,
		static_cast<float>( ( order / 13 ) % 7 ) * 0.21f,
		static_cast<float>( order % 5 ) * 0.11f
	};
	return t;
}

inline static void buildSyntheticForest(EntityRegistry& reg, TransformStorage& ts, u32 targetCount, u32 branchingFactor)
{
	std::vector<EntityId> frontier;
	u32 order = 0;

	const u32 rootCount = std::min<u32>(targetCount, std::max<u32>(1, branchingFactor));
	for (u32 i = 0; i < rootCount && order < targetCount; ++i)
	{
		EntityId e = reg.create();
		ts.create(e, makeTestTransform(order++));
		frontier.push_back(e);
	}

	size_t frontierIdx = 0;
	while (order < targetCount && frontierIdx < frontier.size())
	{
		EntityId parent = frontier[frontierIdx++];
		for (u32 c = 0; c < branchingFactor && order < targetCount; ++c)
		{
			EntityId e = reg.create();
			ts.create(e, makeTestTransform(order++), parent);
			frontier.push_back(e);
		}
	}
}

inline static float maxMatrixDiff(const Mat4f& a, const Mat4f& b)
{
	float worst = 0.f;

	const float* pa = a.data();
	const float* pb = b.data();

	for (int i = 0; i < 16; ++i)
		worst = std::max(worst, std::fabs(pa[i] - pb[i]));

	return worst;
}
