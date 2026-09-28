#pragma once
#include <gfx/render_extraction.h>
#include <gfx/cascade_shadow.h>
#include <array>

namespace imp::gfx
{
	class ITlas;
	class IBuffer;
}

namespace imp::engine
{
	class IRenderScene
	{
	public:
		virtual ~IRenderScene() = default;

		virtual const gfx::RenderExtraction& extraction() const = 0;
		virtual const std::array<gfx::CascadeData, gfx::kCascadeCount>& cascades() const = 0;
		virtual const gfx::CascadeConfig& cascadeConfig() const = 0;

		virtual const gfx::ITlas* staticTlas() const = 0;
		virtual gfx::IBuffer* ddgiInstanceMaterials() const = 0;

		virtual gfx::ModelRegistry& modelRegistry() = 0;
		virtual const math::Vec3f& sunDirection() const = 0;
	};
}
