#include <core/log/log.h>
#include <core/core.h>
#include <core/math/math.h>
#include <iostream>

int main()
{
	imp::log::Logger::get().initialise();

	LOG_INFO("Sandbox", "App starting...");

	if (true)
		LOG_FATAL("Sandbox", "if (true) returned true, shocker");

	// Logging with formatting
	LOG_DEBUG("Sandbox", "A piano has {} keys", 88);

	imp::math::Vec3f a(1, 2, 3);
	LOG_DEBUG("Math", "Vec3f: ({}, {}, {})", a.x, a.y, a.z);

	while (true);

	imp::log::Logger::get().shutdown();

	return 0;
}