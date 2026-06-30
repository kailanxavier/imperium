#include <core/log/log.h>
#include <core/core.h>
#include <iostream>

int main()
{
	imp::log::Logger::get().initialise();

	LOG_INFO("Engine", "Engine starting");

	if (0 != 1)
		LOG_FATAL("Engine", "0 was not equal to 1");

	while (true);

	return 0;
}