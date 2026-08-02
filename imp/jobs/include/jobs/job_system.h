#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <core/types/int_types.h>

// This is a VERY rough V1 job system, just a plain mutex + condvar queue,
// not a lock free or work stealing scheduler. We should revisit only if profiling
// shows queue contention mattering

namespace imp::jobs
{
	namespace detail
	{
		struct CounterState
		{
			std::atomic<u32> pending{ 0 };
			std::mutex mutex;
			std::condition_variable cv;
		};
	}

	class JobCounter
	{
	public:
		JobCounter() = default;
		bool isValid() const noexcept { return static_cast<bool>( m_state ); }

	private:
		friend class JobSystem;
		explicit JobCounter(std::shared_ptr<detail::CounterState> state) : m_state(std::move(state)) {}

		std::shared_ptr<detail::CounterState> m_state;
	};

	class JobSystem
	{
	public:
		JobSystem() = default;
		~JobSystem();

		JobSystem(const JobSystem&) = delete;
		JobSystem& operator=(const JobSystem&) = delete;

		// workerCount = 0 means hardware_concurrency() - 1.
		// Returns false if already running.
		bool initialise(u32 workerCount = 0);

		// Joins all workers. Any jobs still queue are dropped.
		// Callers are expected to have wait()'d on everything 
		// they care about shutting down.
		void shutdown();

		bool isRunning() const noexcept { return m_running; }
		u32 workerCount() const noexcept { return static_cast<u32>(m_workers.size()); }

		// Splits [0, count] into up to workerCount() chunks, each at least
		// minChunkSize wide, and runs fn(start, end) across workers.
		// Blocks until every chunk completes. If the system wasn't running,
		// falls back to calling fn(0, count) inline rather than deadlocking 
		// on an empty pool.
		void parallelFor(u32 count, u32 minChunkSize, std::function<void(u32 start, u32 end)> fn);

		// Fire and forget: queues fn, return immediately with a counter 
		// that reaches zero once fn has run. wait() blocks until then
		JobCounter dispatch(std::function<void()> fn);

		// Blocks until every job associated with counter has completed.
		// No-op on a default-constructed counter.
		void wait(const JobCounter& counter);

	private:
		struct QueueJob
		{
			std::function<void()> fn;
			std::shared_ptr<detail::CounterState> counter;
		};

		void enqueue(QueueJob job);

		// Pushes every job in one lock acquisition, then wakes
		// up exactly jobs.size() workers.
		void enqueueBatch(std::vector<QueueJob> jobs);
		void workerLoop();

		std::vector<std::thread> m_workers;
		std::deque<QueueJob> m_queue;
		std::mutex m_queueMutex;
		std::condition_variable m_queueCV;
		bool m_stopping = false;
		bool m_running = false;
	};
}
