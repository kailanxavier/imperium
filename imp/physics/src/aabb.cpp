#include <physics/collider/aabb.h>
#include <algorithm>
#include <cmath>

namespace imp::physics
{
	namespace
	{
		constexpr float kEpsF = 1e-8f;
	}

	AABB AABB::transformed(const math::Mat4f& m) const
	{
		constexpr int kCornerCount = 8;
		const math::Vec3f corners[kCornerCount] = {
			{ min.x, min.y, min.z }, { max.x, min.y, min.z },
			{ min.x, max.y, min.z }, { max.x, max.y, min.z },
			{ min.x, min.y, max.z }, { max.x, min.y, max.z },
			{ min.x, max.y, max.z }, { max.x, max.y, max.z },
		};

		math::Vec3f newMin = math::transformPoint(m, corners[0]);
		math::Vec3f newMax = newMin;

		for (int i = 1; i < kCornerCount; ++i)
		{
			const math::Vec3f p = math::transformPoint(m, corners[i]);
			newMin.x = std::min(newMin.x, p.x); newMax.x = std::max(newMax.x, p.x);
			newMin.y = std::min(newMin.y, p.y); newMax.y = std::max(newMax.y, p.y);
			newMin.z = std::min(newMin.z, p.z); newMax.z = std::max(newMax.z, p.z);
		}

		return AABB{ newMin, newMax };
	}

	bool AABB::rayIntersect(const math::Vec3f& rayOrigin, const math::Vec3f& rayDir, float maxDistance, float& outT) const
	{
		float tMin = 0.f;
		float tMax = maxDistance;

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(rayDir[axis]) < kEpsF)
			{
				if (rayOrigin[axis] < min[axis] || rayOrigin[axis] > max[axis])
					return false;
				continue;
			}

			const float invDir = 1.f / rayDir[axis];
			float t0 = ( min[axis] - rayOrigin[axis] ) * invDir;
			float t1 = ( max[axis] - rayOrigin[axis] ) * invDir;
			if (t0 > t1)
				std::swap(t0, t1);

			tMin = std::max(tMin, t0);
			tMax = std::min(tMax, t1);

			if (tMin > tMax)
				return false;
		}

		outT = tMin;
		return true;
	}
}
