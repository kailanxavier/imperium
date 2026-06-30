#include <core/log/log.h>
#include <core/core.h>
#include <iostream>

int main()
{
	imp::log::Logger::get().initialise();

	LOG_INFO("Sandbox", "App starting...");

	if (true)
		LOG_FATAL("Sandbox", "if (true) returned true, shocker");

	// Logging with formatting
	LOG_DEBUG("Sandbox", "A piano has {} keys", 88);
	LOG_DEBUG("Math", "Vec3f: ({}, {}, {})", 1.f, 2.f, 3.f);

	while (true);

	imp::log::Logger::get().shutdown();

	return 0;
}