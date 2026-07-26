#include <ecs/entity.h>
#include <gtest/gtest.h>

using namespace imp::ecs;

TEST(Entity, CreateGivesIncreasingIndicesWhenNoReuse)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	EntityId b = reg.create();
	EntityId c = reg.create();

	EXPECT_TRUE(a.index == 0);
	EXPECT_TRUE(b.index == 1);
	EXPECT_TRUE(c.index == 2);
	EXPECT_TRUE(reg.aliveCount() == 3);
}

TEST(Entity, NewEntitiesAreAlive)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	EXPECT_TRUE(reg.isAlive(a));
}

TEST(Entity, DestroyMarksDeadAndStaleHandleDetected)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	reg.destroy(a);

	EXPECT_FALSE(reg.isAlive(a));
	EXPECT_TRUE(reg.aliveCount() == 0);
}

TEST(Entity, DestroyedSlotIsUsedWithBumpedGeneration)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	reg.destroy(a);

	EntityId b = reg.create();

	// Index should be the same
	EXPECT_TRUE(a.index == b.index);

	// But generation should be different
	EXPECT_FALSE(a.generation == b.generation);

	EXPECT_FALSE(reg.isAlive(a));
	EXPECT_TRUE(reg.isAlive(b));
}

TEST(Entity, DoubleDestroyIsSafeNoop)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	reg.destroy(a);
	reg.destroy(a);

	EXPECT_TRUE(reg.aliveCount() == 0);
}

TEST(Entity, InvalidAndDefaultEntityAreNeverAlive)
{
	EntityRegistry reg;
	EXPECT_FALSE(reg.isAlive(kInvalidEntity));
	EXPECT_FALSE(reg.isAlive(EntityId{}));

	// Out of range index on an empty registry
	EntityId bogus{ 42, 0 };
	EXPECT_FALSE(reg.isAlive(bogus));
}

TEST(Entity, FreeListIsLastInFirstOut)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	EntityId b = reg.create();
	EntityId c = reg.create();

	reg.destroy(b);
	reg.destroy(c);

	EntityId d = reg.create(); // should reuse index 2
	EntityId e = reg.create(); // should reuse index 1

	EXPECT_TRUE(d.index == 2);
	EXPECT_TRUE(e.index == 1);
	EXPECT_TRUE(reg.isAlive(a));
	EXPECT_TRUE(reg.isAlive(d));
	EXPECT_TRUE(reg.isAlive(e));
}

TEST(Entity, CapacityGrowsOnlyWhenFreeListEmpty)
{
	EntityRegistry reg;
	EntityId a = reg.create();
	reg.destroy(a);
	EntityId b = reg.create();

	EXPECT_TRUE(reg.capacity() == 1);
	(void)b;

	// No free slots left, must grow
	EntityId c = reg.create();
	EXPECT_TRUE(reg.capacity() == 2);
	(void)c;
}
