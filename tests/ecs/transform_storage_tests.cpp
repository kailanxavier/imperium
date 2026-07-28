#include <gtest/gtest.h>

#include <ecs/entity.h>
#include <ecs/transform_storage.h>

#include <core/math/math.h>
#include "ecs_test_helper.h"

using namespace imp::ecs;
using namespace imp::math;

TEST(TransformStorage, RootWorldMatrixEqualsLocal)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId e = reg.create();
	Transform t;
	t.position = Vec3f{ 1.f, 2.f, 3.f };
	ts.create(e, t);

	ts.updateWorldMatrices();

	const Mat4f& w = ts.worldMatrix(e);
	EXPECT_NEAR(w(0, 3), 1.f, kEpsF);
	EXPECT_NEAR(w(1, 3), 2.f, kEpsF);
	EXPECT_NEAR(w(2, 3), 3.f, kEpsF);
}

TEST(TransformStorage, ChildWorldMatrixComposesWithParent)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId parent = reg.create();
	Transform pt;
	pt.position = Vec3f{ 10.f, 0.f, 0.f };
	ts.create(parent, pt);

	EntityId child = reg.create();
	Transform ct;
	ct.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(child, ct, parent);

	ts.updateWorldMatrices();

	const Mat4f& w = ts.worldMatrix(child);
	EXPECT_NEAR(w(0, 3), 11.f, kEpsF);
	EXPECT_TRUE(ts.depthOf(child) == 1);
	EXPECT_TRUE(ts.depthOf(parent) == 0);
	EXPECT_TRUE(ts.parentOf(child) == parent);
}

TEST(TransformStorage, DenseArrayStaysDepthOrderedRegardlessOfInsertionOrder)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId root = reg.create();
	ts.create(root, Transform{});

	EntityId child = reg.create();
	ts.create(child, Transform{}, root);

	EntityId grandchild = reg.create();
	ts.create(grandchild, Transform{}, child);

	EntityId root2 = reg.create();
	ts.create(root2, Transform{});

	for (size_t i = 1; i < ts.m_depth.size(); ++i)
		EXPECT_TRUE(ts.m_depth[i - 1] <= ts.m_depth[i]);

	EXPECT_TRUE(ts.size() == 4);
}

TEST(TransformStorage, DirtyPropagatesToChildrenWithoutExplicitMark)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId parent = reg.create();
	ts.create(parent, Transform{});

	EntityId child = reg.create();
	Transform ct;
	ct.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(child, ct, parent);

	ts.updateWorldMatrices();

	Transform newParentT;
	newParentT.position = Vec3f{ 5.f, 0.f, 0.f };
	ts.setLocalTransform(parent, newParentT);
	ts.updateWorldMatrices();

	const Mat4f& w = ts.worldMatrix(child);
	EXPECT_NEAR(w(0, 3), 6.f, kEpsF);
}

TEST(TransformStorage, ContainsAndDestroyLeaf)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId e = reg.create();
	ts.create(e, Transform{});
	EXPECT_TRUE(ts.contains(e));

	ts.destroy(e);
	EXPECT_FALSE(ts.contains(e));
	EXPECT_TRUE(ts.size() == 0);
}

TEST(TransformStorage, StaleHandleNotContainedAfterRegistryDestroyAndReuse)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId a = reg.create();
	ts.create(a, Transform{});
	ts.destroy(a);
	reg.destroy(a);

	EntityId b = reg.create(); // a slot, bumped gen
	ts.create(b, Transform{});

	EXPECT_FALSE(ts.contains(a)); // stale handle from before reuse must not alias
	EXPECT_TRUE(ts.contains(b));
}

TEST(TransformStorage, ComputeDepthRanges)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId r1 = reg.create(); ts.create(r1, Transform{});
	EntityId r2 = reg.create(); ts.create(r2, Transform{});
	EntityId c1 = reg.create(); ts.create(c1, Transform{}, r1);
	EntityId c2 = reg.create(); ts.create(c2, Transform{}, r2);
	EntityId gc = reg.create(); ts.create(gc, Transform{}, c1);
	(void)c2;
	(void)gc;

	ts.rebuildDepthRanges();
	const auto& ranges = ts.depthRanges();

	EXPECT_EQ(ranges.size(), 3u);
	EXPECT_EQ(ranges[0].second - ranges[0].first, 2u);
	EXPECT_EQ(ranges[1].second - ranges[1].first, 2u);
	EXPECT_EQ(ranges[2].second - ranges[2].first, 1u);
}
