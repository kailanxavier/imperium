#pragma once
#include "log.h"

namespace imp::log
{
	class FileSink : public ILogSink
	{
		std::string m_path;
		std::FILE* m_file = nullptr;

	public:
		explicit FileSink(const std::string& p) : m_path(p)
		{
			auto err = fopen_s(&m_file, m_path.c_str(), "a");
			if (err != 0)
				m_file = nullptr;
		}
		~FileSink()
		{
			if (m_file)
				std::fclose(m_file);
		}

		void write(const LogEntry& e) override
		{
			if (!m_file) return;
			std::fprintf(m_file, "[%s] [%s] %s (%s:%d) [%s]\n",
				toString(e.level),
				e.category.c_str(),
				e.message.c_str(),
				e.file.c_str(),
				e.line,
				Logger::formattedTimestamp(e.timestamp).c_str()
			);

			if (e.level >= LogLevel::Error)
				std::fflush(m_file);
		}

		void flush() override
		{
			if (m_file)
				std::fflush(m_file);
		}
	};
}