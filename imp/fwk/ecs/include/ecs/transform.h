#pragma once

#include <core/math/math.h>

namespace imp::ecs
{
	struct Transform
	{
		math::Vec3f position = math::Vec3f::zero();
		math::Quaternionf rotation = math::Quaternionf::identity();
		math::Vec3f scale = math::Vec3f::one();
	};
}
