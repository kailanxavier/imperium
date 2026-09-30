#include <gtest/gtest.h>

#include <core/config/cvar.h>
#include <sky/sky_cvars.h>

using namespace imp;
using namespace imp::sky;

TEST(SkyCVars, RegisterSettingsAsCVars)
{
	SkyCVars cvars;

	const auto tod = CVarRegistry::get().get("sky.time_of_day");
	ASSERT_TRUE(tod.has_value());
	EXPECT_EQ(tod->kind, CVarKind::Float);

	const auto cycle = CVarRegistry::get().get("sky.cycle_enabled");
	ASSERT_TRUE(cycle.has_value());
	EXPECT_EQ(cycle->kind, CVarKind::Bool);
}

TEST(SkyCVars, EditingACVarOverridesTheSettings)
{
	SkyCVars cvars;
	SkySettings settings{};
	cvars.push(settings);

	ASSERT_TRUE(CVarRegistry::get().setFloat("sky.sun.intensity", 123.f));
	ASSERT_TRUE(CVarRegistry::get().setBool("sky.cycle_enabled", false));

	cvars.pull(settings);
	EXPECT_FLOAT_EQ(settings.sunIntensity, 123.f);
	EXPECT_FALSE(settings.cycleEnabled);
}
