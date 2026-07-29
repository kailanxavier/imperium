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

TEST(TransformStorage, UpdateOrderTranslatesToCorrectDenseIndices)
{
	EntityRegistry reg;
	TransformStorage ts;
	EntityId r1 = reg.create(); ts.create(r1, Transform{});
	EntityId r2 = reg.create(); ts.create(r2, Transform{});
	EntityId c1 = reg.create(); ts.create(c1, Transform{}, r1);
	EntityId c2 = reg.create(); ts.create(c2, Transform{}, r2);
	EntityId gc = reg.create(); ts.create(gc, Transform{}, c1);
	(void)c2;

	ts.rebuildUpdateOrder();
	const auto& order = ts.updateOrder();
	const auto& ranges = ts.depthRanges();

	ASSERT_EQ(ranges.size(), 3u);

	for (u32 pos = ranges[0].first; pos < ranges[0].second; ++pos)
		EXPECT_TRUE(ts.m_depth[order[pos]] == 0);
	for (u32 pos = ranges[1].first; pos < ranges[1].second; ++pos)
		EXPECT_TRUE(ts.m_depth[order[pos]] == 1);
	for (u32 pos = ranges[2].first; pos < ranges[2].second; ++pos)
		EXPECT_TRUE(ts.m_depth[order[pos]] == 2);
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

TEST(TransformStorage, CascadingDestroyRemovesEntireSubtree)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId root = reg.create();
	ts.create(root, Transform{});
	EntityId child = reg.create();
	ts.create(child, Transform{}, root);
	EntityId grandchild = reg.create();
	ts.create(grandchild, Transform{}, child);

	EntityId sibling = reg.create(); // unrelated, must survive
	ts.create(sibling, Transform{});

	ts.destroy(root);

	EXPECT_FALSE(ts.contains(root));
	EXPECT_FALSE(ts.contains(child));
	EXPECT_FALSE(ts.contains(grandchild));
	EXPECT_TRUE(ts.contains(sibling));
	EXPECT_TRUE(ts.size() == 1);
}

TEST(TransformStorage, DestroyingMidHierarchyEntityCascadesOnlyToItsDescendants)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId root = reg.create();
	ts.create(root, Transform{});
	EntityId mid = reg.create();
	ts.create(mid, Transform{}, root);
	EntityId leaf = reg.create();
	ts.create(leaf, Transform{}, mid);

	ts.destroy(mid);

	EXPECT_TRUE(ts.contains(root));
	EXPECT_FALSE(ts.contains(mid));
	EXPECT_FALSE(ts.contains(leaf));
	EXPECT_TRUE(ts.size() == 1);
}

TEST(TransformStorage, DestroyingOneChildLeavesSiblingIntact)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId parent = reg.create();
	ts.create(parent, Transform{});

	EntityId childA = reg.create();
	ts.create(childA, Transform{}, parent);

	EntityId childB = reg.create();
	Transform bt;
	bt.position = Vec3f{ 3.f, 0.f, 0.f };
	ts.create(childB, bt, parent);

	ts.destroy(childA);

	EXPECT_FALSE(ts.contains(childA));
	EXPECT_TRUE(ts.contains(childB));
	EXPECT_TRUE(ts.parentOf(childB) == parent);
	EXPECT_TRUE(ts.size() == 2);

	ts.updateWorldMatrices();
	const Mat4f& w = ts.worldMatrix(childB);
	EXPECT_NEAR(w(0, 3), 3.f, kEpsF);
}

TEST(TransformStorage, SwapRemoveFixesUpChildrenOfMovedEntity)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId a = reg.create();
	ts.create(a, Transform{});

	EntityId b = reg.create();
	Transform bt;
	bt.position = Vec3f{ 5.f, 0.f, 0.f };
	ts.create(b, bt);

	EntityId c = reg.create();
	Transform ct;
	ct.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(c, ct, b);

	ts.destroy(a);

	EXPECT_TRUE(ts.contains(b));
	EXPECT_TRUE(ts.contains(c));
	EXPECT_TRUE(ts.parentOf(c) == b);
	EXPECT_TRUE(ts.depthOf(c) == 1);

	ts.updateWorldMatrices();
	const Mat4f& w = ts.worldMatrix(c);
	EXPECT_NEAR(w(0, 3), 6.f, kEpsF);
}

TEST(TransformStorage, SwapRemoveKeepsHierarchyConsistentUnderChurn)
{
	EntityRegistry reg;
	TransformStorage ts;

	EntityId bRoot = reg.create();
	Transform brt;
	brt.position = Vec3f{ 100.f, 0.f, 0.f };
	ts.create(bRoot, brt);

	EntityId bChild = reg.create();
	Transform bct;
	bct.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(bChild, bct, bRoot);

	EntityId bGrandchild = reg.create();
	Transform bgt;
	bgt.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(bGrandchild, bgt, bChild);

	for (int round = 0; round < 3; ++round)
	{
		EntityId aRoot = reg.create();
		ts.create(aRoot, Transform{});
		EntityId aChild = reg.create();
		ts.create(aChild, Transform{}, aRoot);
		ts.destroy(aRoot); // cascades to aChild too

		EXPECT_FALSE(ts.contains(aRoot));
		EXPECT_FALSE(ts.contains(aChild));

		EXPECT_TRUE(ts.contains(bRoot));
		EXPECT_TRUE(ts.contains(bChild));
		EXPECT_TRUE(ts.contains(bGrandchild));
		EXPECT_TRUE(ts.parentOf(bChild) == bRoot);
		EXPECT_TRUE(ts.parentOf(bGrandchild) == bChild);
		EXPECT_TRUE(ts.depthOf(bRoot) == 0);
		EXPECT_TRUE(ts.depthOf(bChild) == 1);
		EXPECT_TRUE(ts.depthOf(bGrandchild) == 2);

		ts.updateWorldMatrices();
		const Mat4f& w = ts.worldMatrix(bGrandchild);
		EXPECT_NEAR(w(0, 3), 102.f, kEpsF);
	}

	EXPECT_TRUE(ts.size() == 3);
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

	ts.rebuildUpdateOrder();
	const auto& ranges = ts.depthRanges();

	EXPECT_EQ(ranges.size(), 3u);
	EXPECT_EQ(ranges[0].second - ranges[0].first, 2u);
	EXPECT_EQ(ranges[1].second - ranges[1].first, 2u);
	EXPECT_EQ(ranges[2].second - ranges[2].first, 1u);
}
