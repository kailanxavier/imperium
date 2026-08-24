#include "script_test_helper.h"

#include <ecs/world.h>
#include <script/system.h>

#include <gtest/gtest.h>

using namespace imp;
using namespace imp::ecs;
using namespace imp::script;

namespace
{
	constexpr const char* kMarkerScript = R"lua(
		local M = {}

		function M.OnInit(self, entity)
			entity:SetPosition(1, 0, 0)
		end

		function M.OnUpdate(self, entity, dt)
			entity:SetPosition(2, 0, 0)
		end

		function M.OnDestroy(self, entity)
			entity:SetPosition(3, 0, 0)
		end

		return M
	)lua";
}

class ScriptSystemTest : public ScriptFsFixture
{
protected:
	EntityId spawn(const std::string& scriptVirtualPath, bool wantsTick)
	{
		const EntityId e = world.createEntity();
		world.transforms.create(e, Transform{});
		world.scripts.create(e, scriptVirtualPath, wantsTick);
		return e;
	}

	World world;
};

TEST_F(ScriptSystemTest, OnInitFiresOnFirstUpdateForNonTickingEntity)
{
	const std::string path = writeScript("marker.lua", kMarkerScript);
	const EntityId e = spawn(path, /*wantsTick=*/false);

	ScriptSystem system(vfs);
	system.update(world, 0.016f);

	EXPECT_FLOAT_EQ(world.transforms.localTransform(e).position.x, 1.f);
	EXPECT_EQ(system.liveInstanceCount(), 1u);
}

TEST_F(ScriptSystemTest, OnUpdateOnlyFiresForEntitiesThatOptIntoTicking)
{
	const std::string path = writeScript("marker.lua", kMarkerScript);
	const EntityId ticking = spawn(path, /*wantsTick=*/true);
	const EntityId idle = spawn(path, /*wantsTick=*/false);

	ScriptSystem system(vfs);
	system.update(world, 0.016f);

	EXPECT_FLOAT_EQ(world.transforms.localTransform(ticking).position.x, 2.0f);
	EXPECT_FLOAT_EQ(world.transforms.localTransform(idle).position.x, 1.0f);
}

TEST_F(ScriptSystemTest, OnUpdateContinuesFiringEveryFrameForTickingEntities)
{
	const std::string path = writeScript("marker.lua", kMarkerScript);
	const EntityId e = spawn(path, /*wantsTick=*/true);

	ScriptSystem system(vfs);
	system.update(world, 0.016f);
	world.transforms.setLocalTransform(e, Transform{}); // reset the marker
	system.update(world, 0.016f);

	EXPECT_FLOAT_EQ(world.transforms.localTransform(e).position.x, 2.0f);
}

TEST_F(ScriptSystemTest, OnDestroyFiresWithCorrectHookName)
{
	const std::string path = writeScript("marker.lua", kMarkerScript);
	const EntityId e = spawn(path, /*wantsTick=*/false);

	ScriptSystem system(vfs);
	system.update(world, 0.016f);

	system.onEntityDestroyed(e);

	EXPECT_FLOAT_EQ(world.transforms.localTransform(e).position.x, 3.0f);
	EXPECT_EQ(system.liveInstanceCount(), 0u);
}

TEST_F(ScriptSystemTest, ScriptWithNoOnUpdateIsSafeToTick)
{
	const std::string path = writeScript("no_update.lua", 
	R"lua(
		local M = {}
		function M.OnInit(self, entity) end
		return M
	)lua");

	const EntityId e = spawn(path, /*wantsTick=*/true);

	ScriptSystem system(vfs);
	EXPECT_NO_FATAL_FAILURE(system.update(world, 0.016f));
}
