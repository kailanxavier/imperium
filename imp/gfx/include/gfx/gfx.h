#pragma once
#include <fwk/gfx_device.h>

#include <memory>

namespace imp::gfx
{
	std::unique_ptr<imp::fwk::IGfxDevice> createDevice();

	const char* Version();
	const char* ActiveBackend();
}