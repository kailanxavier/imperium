#include <jobs/job_system.h>

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace imp::jobs;

TEST(JobSystem, DefaultWorkerCountIsAtLeastOne)
{
	JobSystem js;
	EXPECT_TRUE(js.initialise(0));
	EXPECT_TRUE(js.workerCount() >= 1);
	js.shutdown();
}

TEST(JobSystem, ParallelForCoversEveryElementExactlyOnce)
{
	JobSystem js;
	js.initialise(4);

	const u32 n = 1000;
	std::vector<int> hitCount(n, 0);

	js.parallelFor(n, 16, [&](u32 start, u32 end)
		{
			for (u32 i = start; i < end; ++i)
			{
				hitCount[i]++;
			}
		});

	for (u32 i = 0; i < n; ++i)
	{
		EXPECT_TRUE(hitCount[i] == 1);
	}

	js.shutdown();
}

TEST(JobSystem, ParallelForResultMatchesSerialComputation)
{
	JobSystem js;
	js.initialise(4);

	const u32 n = 2000;
	std::vector<int> input(n);

	std::iota(input.begin(), input.end(), 1);

	std::vector<int> output(n, 0);
	js.parallelFor(n, 32, [&](u32 start, u32 end)
		{
			for (u32 i = start; i < end; ++i)
			{
				output[i] = input[i] * 2;
			}
		});

	for (u32 i = 0; i < n; ++i)
	{
		EXPECT_TRUE(output[i] == input[i] * 2);
	}

	js.shutdown();
}

TEST(JobSystem, ParallelForChunkNeverExceedsWorkerCount)
{
	JobSystem js;
	js.initialise(4);

	std::atomic<u32> concurrentChunks{ 0 };
	std::atomic<u32> maxObservedConcurrency{ 0 };

	js.parallelFor(10000, 1, [&](u32 start, u32 end)
		{
			u32 now = ++concurrentChunks;
			u32 prevMax = maxObservedConcurrency.load();
			while (now > prevMax && !maxObservedConcurrency.compare_exchange_weak(prevMax, now)) {}
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
			--concurrentChunks;

			(void)start;
			(void)end;
		});

	EXPECT_TRUE(maxObservedConcurrency.load() <= js.workerCount());

	js.shutdown();
}

TEST(JobSystem, DispatchCompletesBeforeWaitReturns)
{
	JobSystem js;
	js.initialise(2);

	std::atomic<bool> ran{ false };
	JobCounter c = js.dispatch([&]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			ran.store(true, std::memory_order_release);
		});
	js.wait(c);

	EXPECT_TRUE(ran.load(std::memory_order_acquire));

	js.shutdown();
}

TEST(JobSystem, MultipleIndependentDispatchesAllComplete)
{
	JobSystem js;
	js.initialise(4);

	std::atomic<int> counter{ 0 };
	JobCounter c1 = js.dispatch([&] { counter.fetch_add(1); });
	JobCounter c2 = js.dispatch([&] { counter.fetch_add(1); });
	JobCounter c3 = js.dispatch([&] { counter.fetch_add(1); });

	js.wait(c1);
	js.wait(c2);
	js.wait(c3);

	EXPECT_TRUE(counter.load() == 3);
	js.shutdown();
}

TEST(JobSystem, WaitOnDefaultConstructedCounterIsNoop)
{
	JobSystem js;
	js.initialise(2);

	JobCounter empty;
	js.wait(empty);

	js.shutdown();
}

TEST(JobSystem, DispatchBeforeInitialiseRunInlineWithoutDeadlock)
{
	JobSystem js; // never initialised

	bool ran = false;
	JobCounter c = js.dispatch([&] { ran = true; });
	js.wait(c);

	EXPECT_TRUE(ran);
}

TEST(JobSystem, ParallelForBeforeInitialiseRunsInlineWithoutDeadlock)
{
	JobSystem js;

	std::vector<int> out(10, 0);
	js.parallelFor(10, 2, [&](u32 start, u32 end)
		{
			for (u32 i = start; i < end; ++i) out[i] = static_cast<int>(i);
		});

	for (int i = 0; i < 10; ++i)
	{
		EXPECT_TRUE(out[static_cast<size_t>(i)] == i);
	}
}

TEST(JobSystem, ShutdownThenReinitialiseStillRunsJobs)
{
	JobSystem js;
	js.initialise(2);
	js.shutdown();

	EXPECT_TRUE(js.initialise(2));

	std::atomic<bool> ran{ false };
	JobCounter c = js.dispatch([&] { ran.store(true); });
	js.wait(c);
	EXPECT_TRUE(ran.load());

	js.shutdown();
}

TEST(JobSystem, ParallelForRunsInlineWhenWorloadTooSmallToSplit)
{
	JobSystem js;
	js.initialise(8);

	const std::thread::id callerId = std::this_thread::get_id();
	std::thread::id sawId{};

	js.parallelFor(5, 1000, [&](u32 start, u32 end)
		{
			sawId = std::this_thread::get_id();
			(void)start;
			(void)end;
		});

	EXPECT_TRUE(sawId == callerId);
	js.shutdown();
}

TEST(JobSystem, BenchTinyWorkloadOverhead)
{
	JobSystem js;
	js.initialise(8);

	const int iterations = 50000;

	{
		const auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < iterations; ++i)
			js.parallelFor(4, 1000, [](u32 start, u32 end) {});

		const auto t1 = std::chrono::steady_clock::now();
		const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
		GTEST_LOG_(INFO) << "[Bench] inline fast path (chunkCount=1): " << ms << "ms total, "
			<< ( ms * 1000.0 ) / iterations << "us/call\n";
	}

	{
		const auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < iterations; ++i)
			js.parallelFor(4, 1, [](u32 start, u32 end) {});

		const auto t1 = std::chrono::steady_clock::now();
		const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
		GTEST_LOG_(INFO) << "[Bench] queued path (chunkCount=4): " << ms << "ms total, "
			<< ( ms * 1000.0 ) / iterations << "us/call\n";
	}

	js.shutdown();
}
