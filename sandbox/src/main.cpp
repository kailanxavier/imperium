#include <core/log/log.h>
#include <core/core.h>
#include <core/math/math.h>
#include <core/memory/allocator.h>
#include <iostream>

int main()
{
	imp::log::Logger::get().initialise();

	LOG_INFO("Sandbox", "App starting...");

	imp::memory::HeapAllocator heap("main_heap");
	imp::math::Vec3f* p = IMP_NEW(imp::math::Vec3f, heap, imp::memory::MemTag::Renderer, 1.f, 2.f, 3.f);

	LOG_INFO("Memory", "Created pointer to Vec3: ({}, {}, {})", p->x, p->y, p->z);

	IMP_DELETE(imp::math::Vec3f, heap, p, imp::memory::MemTag::Renderer);

	while (true);

	imp::log::Logger::get().shutdown();

	return 0;
}