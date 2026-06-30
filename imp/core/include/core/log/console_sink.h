#pragma once
#include "log.h"

namespace imp::log
{
	class ConsoleSink : public ILogSink
	{
		void write(const LogEntry& e) override
		{
			auto& stream = ( e.level >= LogLevel::Error ) ? std::cerr : std::cout;

			// Build sink log string
			stream
				<< "[" << ToString(e.level) << "] "
				<< "[" << e.category << "] "
				<< e.message << " "
				<< "(" << e.file << ":" << e.line << ")\n";
		}
	};
}