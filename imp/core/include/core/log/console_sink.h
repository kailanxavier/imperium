#pragma once
#include "log.h"

namespace imp::log
{
	class ConsoleSink : public ILogSink
	{
		void write(const LogEntry& e) override
		{
			static const char* RESET = "\033[0m";
			static const char* RED = "\033[31m";
			static const char* GREEN = "\033[32m";
			static const char* YELLOW = "\033[33m";
			static const char* CYAN = "\033[36m";
			static const char* BLUE = "\033[94m";
			static const char* WHITE = "\033[37m";
			static const char* BOLD = "\033[1m";

			const char* colour = WHITE;
			if (e.level == LogLevel::Info) colour = BLUE;
			if (e.level == LogLevel::Warning) colour = YELLOW;
			else if (e.level >= LogLevel::Error) colour = RED;

			auto& stream = ( e.level >= LogLevel::Error ) ? std::cerr : std::cout;

			// Build sink log string
			stream
				<< colour
				<< "[" << toString(e.level) << "] "
				<< "[" << e.category << "] "
				<< e.message << " "
				<< "(" << e.file << ":" << e.line << ")"
				<< RESET << std::endl;
		}
	};
}