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

		static const char* ToString(LogLevel level)
		{
			switch (level)
			{
			case LogLevel::Trace:	return "TRACE";
			case LogLevel::Debug:	return "DEBUG";
			case LogLevel::Info:	return "INFO";
			case LogLevel::Warning: return "WARN";
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

		void initialise(const std::string& logFilePath = "engine.log");
		void shutdown();

		void addSink(std::shared_ptr<ILogSink> sink);
		void removeSink(std::shared_ptr<ILogSink> sink);

		void log(LogLevel level, std::string_view category, std::string_view message, std::string_view file = "", int line = 0);

	private:
		Logger() = default;
		~Logger() { shutdown(); }
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		void processQueue();

		std::vector<std::shared_ptr<ILogSink>> m_sinks;
		std::queue<LogEntry> m_messageQueue;
		std::mutex m_queueMutex;
		std::condition_variable m_queueCV;
		std::atomic<bool> m_running{ false };
		std::thread m_workerThread;
	};

	// Convenience macros
	#define LOG_TRACE(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Trace, cat, msg, __FILE__, __LINE__)
	#define LOG_DEBUG(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Debug, cat, msg, __FILE__, __LINE__)
	#define LOG_INFO(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Info, cat, msg, __FILE__, __LINE__)
	#define LOG_WARN(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Warning, cat, msg, __FILE__, __LINE__)
	#define LOG_ERROR(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Error, cat, msg, __FILE__, __LINE__)
	#define LOG_FATAL(cat, msg) imp::log::Logger::get().log(imp::log::LogLevel::Fatal, cat, msg, __FILE__, __LINE__)
}