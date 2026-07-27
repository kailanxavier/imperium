#include <jobs/job_system.h>

namespace imp::jobs
{
	JobSystem::~JobSystem()
	{
		shutdown();
	}

	bool JobSystem::initialise(u32 workerCount)
	{
		if (m_running)
			return false;

		if (workerCount == 0)
		{
			const unsigned int hw = std::thread::hardware_concurrency();
			workerCount = ( hw > 1 ) ? ( hw - 1 ) : 1;
		}

		m_stopping = false;
		m_running = true;

		m_workers.reserve(workerCount);
		for (u32 i = 0; i < workerCount; ++i)
			m_workers.emplace_back([this] { workerLoop(); });

		return true;
	}

	void JobSystem::shutdown()
	{
		if (!m_running)
			return;

		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_stopping = true;
		}

		m_queueCV.notify_all();

		for (auto& worker : m_workers)
			if (worker.joinable())
				worker.join();

		m_workers.clear();

		{
			// Anything left here never ran. Expected if the called 
			// never wait()'d on what they dispatched before shutting down.
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_queue.clear();
		}

		m_running = false;
	}

	void JobSystem::parallelFor(u32 count, u32 minChunkSize, std::function<void(u32 start, u32 end)> fn)
	{
		if (count == 0)
			return;

		if (!m_running)
		{
			fn(0, count);
			return;
		}

		minChunkSize = std::max(minChunkSize, 1u);

		const u32 workerCap = std::max<u32>(1, workerCount());
		u32 chunkCount = ( count + minChunkSize - 1 ) / minChunkSize;
		chunkCount = std::min(chunkCount, workerCap);
		chunkCount = std::max(chunkCount, 1u);

		if (chunkCount == 1)
		{
			fn(0, count);
			return;
		}

		const u32 baseChunk = count / chunkCount;
		const u32 remainder = count % chunkCount;

		auto state = std::make_shared<detail::CounterState>();
		state->pending.store(chunkCount, std::memory_order_relaxed);

		std::vector<QueueJob> jobs;
		jobs.reserve(chunkCount);

		u32 start = 0;
		for (u32 i = 0; i < chunkCount; ++i)
		{
			const u32 thisChunk = baseChunk + ( i < remainder ? 1u : 0u );
			const u32 end = start + thisChunk;
			jobs.push_back(QueueJob{ [fn, start, end] { fn(start, end); }, state });
			start = end;
		}

		enqueueBatch(std::move(jobs));

		wait(JobCounter(state));
	}

	JobCounter JobSystem::dispatch(std::function<void()> fn)
	{
		if (!m_running)
		{
			fn();
			return JobCounter{};
		}

		auto state = std::make_shared<detail::CounterState>();
		state->pending.store(1, std::memory_order_relaxed);
		enqueue(QueueJob{ std::move(fn), state });
		return JobCounter(state);
	}

	void JobSystem::wait(const JobCounter& counter)
	{
		if (!counter.isValid()) return;

		auto state = counter.m_state; // JobCounter is private access friend. Keep local for clarity
		std::unique_lock<std::mutex> lock(state->mutex);
		state->cv.wait(lock, [&state] { return state->pending.load(std::memory_order_acquire) == 0; });
	}

	void JobSystem::enqueue(QueueJob job)
	{
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_queue.push_back(std::move(job));
		}
		m_queueCV.notify_one();
	}

	void JobSystem::enqueueBatch(std::vector<QueueJob> jobs)
	{
		if (jobs.empty())
			return;

		const size_t n = jobs.size();
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			for (auto& job : jobs)
				m_queue.push_back(std::move(job));
		}

		// n wake signals, enough to service every chunk
		// just pushed, no more. Released the lock above
		// so a woken worker doesn't immediately block
		// on a mutex we're still holding.
		for (size_t i = 0; i < n; ++i)
			m_queueCV.notify_one();
	}

	void JobSystem::workerLoop()
	{
		for (;;)
		{
			QueueJob job;
			{
				std::unique_lock<std::mutex> lock(m_queueMutex);
				m_queueCV.wait(lock, [this] { return m_stopping || !m_queue.empty(); });

				if (m_stopping && m_queue.empty())
					return;

				job = std::move(m_queue.front());
				m_queue.pop_front();
			}

			job.fn();

			if (job.counter)
			{
				if (job.counter->pending.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					// We were the last job in this batch to finish. 
					std::lock_guard<std::mutex> lock(job.counter->mutex);
					job.counter->cv.notify_all();
				}
			}
		}
	}
}
