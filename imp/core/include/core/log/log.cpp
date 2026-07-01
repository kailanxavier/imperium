#include "log.h"

// Sink interface implementations
#include "console_sink.h"
#include "file_sink.h"

namespace imp::log
{
	Logger& Logger::get()
	{
		static Logger instance;
		return instance;
	}

	void Logger::initialise(const std::string& logFilePath) noexcept
	{
		if (m_running.exchange(true)) return;

		addSink(std::make_shared<ConsoleSink>());

		if (!logFilePath.empty())
			addSink(std::make_shared<FileSink>(logFilePath));

		m_workerThread = std::thread(&Logger::processQueue, this);
	}

	void Logger::shutdown() noexcept
	{
		if (!m_running.exchange(false)) return;

		m_queueCV.notify_all();
		if (m_workerThread.joinable())
			m_workerThread.join();

		for (auto& sink : m_sinks)
			sink->flush();

		m_sinks.clear();
	}

	void Logger::addSink(std::shared_ptr<ILogSink> sink)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_sinks.push_back(std::move(sink));
	}

	void Logger::removeSink(std::shared_ptr<ILogSink> sink)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_sinks.erase(std::remove(m_sinks.begin(), m_sinks.end(), sink), m_sinks.end());
	}

	void Logger::setGlobalLevel(LogLevel level)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_globalLevel = level;
	}

	void Logger::setCategoryFilter(const std::string& category, LogLevel level)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_categoryFilters[category] = level;
	}

	void Logger::removeCategoryFilter(const std::string& category)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_categoryFilters.erase(category);
	}

	bool Logger::checkFilter(LogLevel level, std::string_view category)
	{
		// Called from log(), meaning m_queueMutex is already locked
		if (level < m_globalLevel)
			return false;

		auto it = m_categoryFilters.find(std::string(category));
		if (it != m_categoryFilters.end())
			return level >= it->second;

		return true;
	}

	void Logger::log(LogLevel level, std::string_view category, std::string_view message, std::string_view file, int line)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		if (!checkFilter(level, category))
			return;

		m_messageQueue.push(LogEntry
		{
			std::chrono::system_clock::now(),
			level,
			std::string(category),
			std::string(message),
			std::string(file),
			line
		});

		m_queueCV.notify_one();
	}

	void Logger::processQueue()
	{
		while (m_running)
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_queueCV.wait(lock, [this] { return !m_messageQueue.empty() || !m_running; });

			// Swap the whole queue to minimise lock time
			std::queue<LogEntry> localQueue;
			std::swap(localQueue, m_messageQueue);
			lock.unlock();

			// Write to all sinks outside the lock
			while (!localQueue.empty())
			{
				const auto& entry = localQueue.front();
				for (auto& sink : m_sinks)
					sink->write(entry);
				localQueue.pop();
			}
		}

		// Final flush to process any remaining entries we may have missed
		std::queue<LogEntry> remaining;
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			std::swap(remaining, m_messageQueue);
		}
		while (!remaining.empty())
		{
			for (auto& sink : m_sinks)
				sink->write(remaining.front());
			remaining.pop();
		}
	}

	std::string Logger::formattedTimestamp(const std::chrono::system_clock::time_point& tp)
	{
		using namespace std::chrono;
		auto ms = duration_cast<milliseconds>( tp.time_since_epoch() ) % 1000;
		auto timer = system_clock::to_time_t(tp);
		std::tm bt;
#ifdef _WIN32
		localtime_s(&bt, &timer);
#else
		localtime_r(&timer, &bt);
#endif
		// Build string
		std::ostringstream oss;
		oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
		oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
		return oss.str();
	}
}