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
			m_file = std::fopen(m_path.c_str(), "a");
		}
		~FileSink()
		{
			if (m_file)
				std::fclose(m_file);
		}

		void write(const LogEntry& e) override
		{
			if (!m_file) return;
			std::fprintf(m_file, "[%s] [%s] %s (%s:%d)\n",
				ToString(e.level),
				e.category.c_str(),
				e.message.c_str(),
				e.file.c_str(),
				e.line
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