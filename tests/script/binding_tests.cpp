#include <ecs/world.h>
#include <script/binding.h>

#include <gtest/gtest.h>
#include <sol/sol.hpp>

using namespace imp;
using namespace imp::ecs;
using namespace imp::script;

namespace
{
	ModelHandle testModelHandle() { return ModelHandle{ 1, 0 }; }
}

class ScriptBindingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		lua.open_libraries(sol::lib::base);
		registerEntityBindings(lua);
	}

	sol::state lua;
	World world;
};

TEST_F(ScriptBindingTest, GetPositionReturnsWorldPositionForAliveEntity)
{
	const EntityId e = world.createEntity();
	Transform t;
	t.position = math::Vec3f{ 1.f, 2.f, 3.f };
	world.transforms.create(e, t);

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script(R"lua(
		local pos = e:GetPosition()
		return pos.x, pos.y, pos.z
	)lua");
	ASSERT_TRUE(result.valid());

	const auto [x, y, z] = result.get<std::tuple<float, float, float>>();
	EXPECT_FLOAT_EQ(x, 1.f);
	EXPECT_FLOAT_EQ(y, 2.f);
	EXPECT_FLOAT_EQ(z, 3.f);
}

TEST_F(ScriptBindingTest, GetPositionReturnsNilForEntityWithoutTransform)
{
	const EntityId e = world.createEntity(); // no transform attached

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script("return e:GetPosition() == nil");
	ASSERT_TRUE(result.valid());
	EXPECT_TRUE(result.get<bool>());
}

TEST_F(ScriptBindingTest, GetPositionReturnsNilForDeadEntity)
{
	const EntityId e = world.createEntity();
	world.transforms.create(e, Transform{});
	world.destroyEntity(e);

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script("return e:GetPosition() == nil");
	ASSERT_TRUE(result.valid());
	EXPECT_TRUE(result.get<bool>());
}

TEST_F(ScriptBindingTest, SetPositionMutatesTheEntityTransform)
{
	const EntityId e = world.createEntity();
	world.transforms.create(e, Transform{});

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script("e:SetPosition(4, 5, 6)");
	ASSERT_TRUE(result.valid());

	const math::Vec3f pos = world.transforms.localTransform(e).position;
	EXPECT_FLOAT_EQ(pos.x, 4.f);
	EXPECT_FLOAT_EQ(pos.y, 5.f);
	EXPECT_FLOAT_EQ(pos.z, 6.f);
}

TEST_F(ScriptBindingTest, SetRenderableVisibleTogglesVisibilityForEntityWithRenderable)
{
	const EntityId e = world.createEntity();
	world.renderables.create(e, testModelHandle(), /*visible=*/false);

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script("e:SetRenderableVisible(true)");
	ASSERT_TRUE(result.valid());

	EXPECT_TRUE(world.renderables.visible(e));
}

TEST_F(ScriptBindingTest, SetRenderableVisibleIsNoOpForEntityWithTransformButNoRenderable)
{
	const EntityId e = world.createEntity();
	world.transforms.create(e, Transform{});
	ASSERT_FALSE(world.renderables.contains(e));

	lua["e"] = ScriptEntityHandle{ e, &world };
	const sol::protected_function_result result = lua.script("e:SetRenderableVisible(true)");
	ASSERT_TRUE(result.valid());

	EXPECT_FALSE(world.renderables.contains(e));
}

TEST_F(ScriptBindingTest, MethodsAreSafeOnDefaultConstructedHandle)
{
	lua["e"] = ScriptEntityHandle{};
	const sol::protected_function_result result = lua.script(
	R"lua(
		e:SetPosition(1, 2, 3)
		e:SetRenderableVisible(true)
		return e:GetPosition() == nil
	)lua");
	ASSERT_TRUE(result.valid());
	EXPECT_TRUE(result.get<bool>());
}
