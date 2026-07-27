#include <ecs/entity.h>
#include <ecs/transform_storage.h>
#include <jobs/job_system.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include "ecs_test_helper.h"

#include <gtest/gtest.h>

using namespace imp::ecs;
using namespace imp::math;
using namespace imp::jobs;

TEST(TransformStorage, ParallelMatchesSerialOnSyntheticForest)
{
	const u32 targetCount = 20000;
	const u32 branching = 5;

	EntityRegistry regSerial;
	TransformStorage tsSerial;
	buildSyntheticForest(regSerial, tsSerial, targetCount, branching);
	tsSerial.updateWorldMatrices();

	EntityRegistry regParallel;
	TransformStorage tsParallel;
	buildSyntheticForest(regParallel, tsParallel, targetCount, branching);

	JobSystem js;
	js.initialise(0);
	tsParallel.updateWorldMatricesParallel(js, 64);
	js.shutdown();

	float worstDiff = 0.f;
	for (u32 i = 0; i < targetCount; ++i)
	{
		const EntityId id{ i, 0 };
		worstDiff = std::max(worstDiff, maxMatrixDiff(tsSerial.worldMatrix(id), tsParallel.worldMatrix(id)));
	}

	EXPECT_TRUE(worstDiff < kEpsFF);
}

TEST(TransformStorage, DirtyPropagatesAcrossMultipleBarriers)
{
	EntityRegistry reg;
	TransformStorage ts;
	JobSystem js;
	js.initialise(4);

	EntityId root = reg.create();
	ts.create(root, Transform{});

	EntityId mid = reg.create();
	ts.create(mid, Transform{}, root);

	EntityId leaf = reg.create();
	Transform lt;
	lt.position = Vec3f{ 1.f, 0.f, 0.f };
	ts.create(leaf, lt, mid);

	ts.updateWorldMatricesParallel(js, 4);

	Transform newRootT;
	newRootT.position = Vec3f{ 9.f, 0.f, 0.f };
	ts.setLocalTransform(root, newRootT);
	ts.updateWorldMatricesParallel(js, 4);

	const Mat4f& w = ts.worldMatrix(leaf);
	EXPECT_NEAR(w(0, 3), 10.f, kEpsF);

	js.shutdown();
}

TEST(TransformStorage, ParallelUpdateIsNoopWhenNothingDirty)
{
	EntityRegistry reg;
	TransformStorage ts;
	JobSystem js;
	js.initialise(2);

	EntityId root = reg.create();
	ts.create(root, Transform{});
	EntityId child = reg.create();
	Transform ct;
	ct.position = Vec3f{ 2.f, 0.f, 0.f };
	ts.create(child, ct, root);

	ts.updateWorldMatricesParallel(js, 4);
	const Mat4f before = ts.worldMatrix(child);

	ts.updateWorldMatricesParallel(js, 4); // nothing changed since last call
	const Mat4f after = ts.worldMatrix(child);

	EXPECT_TRUE(maxMatrixDiff(before, after) < kEpsF);

	js.shutdown();
}

TEST(TransformStorage, StressBenchmarkAndCorrectnessAtScale)
{
	const u32 targetCount = 200000;
	const u32 branching = 8;

	EntityRegistry regSerial;
	TransformStorage tsSerial;
	buildSyntheticForest(regSerial, tsSerial, targetCount, branching);

	const auto t0 = std::chrono::steady_clock::now();
	tsSerial.updateWorldMatrices();
	const auto t1 = std::chrono::steady_clock::now();
	const double serialMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

	EntityRegistry regParallel;
	TransformStorage tsParallel;
	buildSyntheticForest(regParallel, tsParallel, targetCount, branching);

	JobSystem js;
	js.initialise(0);
	const u32 workers = js.workerCount();

	const auto t2 = std::chrono::steady_clock::now();
	tsParallel.updateWorldMatricesParallel(js, 256);
	const auto t3 = std::chrono::steady_clock::now();
	const double parallelMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

	js.shutdown();

	GTEST_LOG_(INFO)
		<< "[Bench] "
		<< targetCount << " entities, branching=" << branching
		<< ", " << workers << " workers: "
		<< "serial=" << serialMs << " ms "
		<< "parallel=" << parallelMs << " ms "
		<< "speedup=" << serialMs / std::max(parallelMs, 0.001)
		<< "x";

	float worstDiff = 0.f;
	for (u32 i = 0; i < targetCount; ++i)
	{
		const EntityId id{ i, 0 };
		worstDiff = std::max(worstDiff, maxMatrixDiff(tsSerial.worldMatrix(id), tsParallel.worldMatrix(id)));
	}

	EXPECT_TRUE(worstDiff < kEpsFF);
}
