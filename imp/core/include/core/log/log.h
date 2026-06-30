#pragma once
#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>

#include "fmt/core.h"

namespace imp::log 
{
	enum class LogLevel
	{
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal,
		Off
	};

	struct LogEntry
	{
		std::chrono::system_clock::time_point timestamp;
		LogLevel level;
		std::string category;
		std::string message;
		std::string file;
		int line;
	};

	class ILogSink
	{
	public:
		virtual ~ILogSink() = default;
		virtual void write(const LogEntry& entry) = 0;
		virtual void flush() {}

		[[nodiscard]] static const char* toString(LogLevel level) noexcept
		{
			switch (level)
			{
			case LogLevel::Trace:	return "TRACE";
			case LogLevel::Debug:	return "DEBUG";
			case LogLevel::Info:	return "INFO";
			case LogLevel::Warning: return "WARNING";
			case LogLevel::Error:	return "ERROR";
			case LogLevel::Fatal:	return "FATAL";
			default:				return "UNKNOWN :(";
			}
		}
	};

	class Logger
	{
	public:
		static Logger& get();

		void initialise(const std::string& logFilePath = "engine.log") noexcept;
		void shutdown() noexcept;

		// Sink management
		void addSink(std::shared_ptr<ILogSink> sink);
		void removeSink(std::shared_ptr<ILogSink> sink);

		// Filtering
		void setGlobalLevel(LogLevel level);
		void setCategoryFilter(const std::string& category, LogLevel level);
		void removeCategoryFilter(const std::string& category);

		// Core logging
		void log(LogLevel level, std::string_view category, std::string_view message, std::string_view file = "", int line = 0);

		template <typename... Args>
		void logFormat(LogLevel level, std::string_view category, std::string_view file,
			int line, fmt::format_string<Args...> fmt, Args&&... args)
		{
			// Filter early for performance
			if (!checkFilter(level, category)) return;

			std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
			log(level, category, msg, file, line);
		}

		static std::string formattedTimestamp(const std::chrono::system_clock::time_point& tp);

	private:
		Logger() = default;
		~Logger() { shutdown(); }
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		void processQueue();
		bool checkFilter(LogLevel level, std::string_view category);

		std::vector<std::shared_ptr<ILogSink>> m_sinks;
		std::queue<LogEntry> m_messageQueue;
		std::mutex m_queueMutex;
		std::condition_variable m_queueCV;
		std::atomic<bool> m_running{ false };
		std::thread m_workerThread;

		LogLevel m_globalLevel = LogLevel::Trace;
		std::unordered_map<std::string, LogLevel> m_categoryFilters;
	};

	// Convenience macros
	// Syntax is ([category], [text]: {}{}, var1, var2) - and the actual values of the vars replace the {}
	#define LOG_TRACE(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Trace, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
	#define LOG_DEBUG(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Debug, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
	#define LOG_INFO(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Info, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
	#define LOG_WARN(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Warning, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
	#define LOG_ERROR(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Error, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
	#define LOG_FATAL(cat, fmt, ...) imp::log::Logger::get().logFormat(imp::log::LogLevel::Fatal, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
}