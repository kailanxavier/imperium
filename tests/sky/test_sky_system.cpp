#include <gtest/gtest.h>

#include <sky/sky_system.h>
#include <cmath>

using namespace imp::sky;

namespace
{
	SkySettings frozen(float hours)
	{
		SkySettings s{};
		s.cycleEnabled = false;
		s.timeOfDayHours = hours;
		return s;
	}

	imp::math::Vec3f toward(const CelestialBody& body)
	{
		return -body.direction;
	}
}

TEST(SkySystem, SunIsHighAndSouthAtNoonInTheNorthernHemisphere)
{
	SkySystem sky(frozen(12.f));
	const imp::math::Vec3f sun = toward(sky.state().sun);

	EXPECT_GT(sun.y, 0.8f);
	EXPECT_LT(sun.z, 0.f);
	EXPECT_NEAR(sun.x, 0.f, 1e-4f);
}

TEST(SkySystem, SunRisesInTheEastAndSetsInTheWest)
{
	SkySystem morning(frozen(6.f));
	SkySystem evening(frozen(18.f));

	EXPECT_GT(toward(morning.state().sun).x, 0.5f);
	EXPECT_LT(toward(evening.state().sun).x, -0.5f);
}
