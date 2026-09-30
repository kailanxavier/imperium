#pragma once

#include <gfx/resources.h>

namespace imp::app
{
	struct RenderTargetFormats
	{
		gfx::TextureFormat colour = gfx::TextureFormat::RGBA16Float;
		gfx::TextureFormat depth = gfx::TextureFormat::Depth32Float;
		gfx::SampleCount samples = gfx::SampleCount::One;
	};
}
