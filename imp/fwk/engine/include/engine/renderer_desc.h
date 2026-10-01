#pragma once

#include "renderer_manifest.h"
#include <gfx/cascade_shadow.h>

namespace imp::gfx { class ModelRegistry; }

namespace imp::engine
{
	struct RenderFeatures
	{
		bool ao = true;
		bool ssgi = true;
		bool ddgi = true;
		bool thermal = true;
		bool taa = true;
		bool bloom = true;
	};

	struct RendererDesc
	{
		RendererManifest manifest{};
		RenderFeatures features{};
		gfx::CascadeConfig cascades{};
		gfx::ModelRegistry* modelRegistry = nullptr;

		bool enableFrustumCulling = true;
	};
}
