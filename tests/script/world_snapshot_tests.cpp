#include <gtest/gtest.h>
#include <protocol/world_snapshot.h>

using namespace imp::protocol;

TEST(WorldSnapshotSerialisation, RoundTripsAScriptComponent)
{
	EntitySnapshotPayload e;
	e.index = 3;
	e.generation = 1;
	e.name = "Sponza";

	ScriptComponentPayload script;
	script.path = "assets/scripts/sponza.lua";
	script.wantsTick = true;
	e.script = script;

	const auto bytes = serialiseWorldSnapshot({ e });
	const auto decoded = deserialiseWorldSnapshot(bytes);

	ASSERT_TRUE(decoded.has_value());
	ASSERT_EQ(decoded->size(), 1u);
	ASSERT_TRUE((*decoded)[0].script.has_value());
	EXPECT_EQ((*decoded)[0].script->path, "assets/scripts/sponza.lua");
	EXPECT_TRUE((*decoded)[0].script->wantsTick);
}

TEST(WorldSnapshotSerialisation, LeavesScriptEmptyForEntitiesWithoutOne)
{
	EntitySnapshotPayload e;
	e.index = 7;
	e.generation = 0;

	const auto bytes = serialiseWorldSnapshot({ e });
	const auto decoded = deserialiseWorldSnapshot(bytes);

	ASSERT_TRUE(decoded.has_value());
	ASSERT_EQ(decoded->size(), 1u);
	EXPECT_FALSE((*decoded)[0].script.has_value());
}
