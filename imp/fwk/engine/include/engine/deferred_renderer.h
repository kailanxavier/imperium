#pragma once

#include <engine/render_context.h>
#include <engine/renderer_desc.h>

#include <camera/camera.h>
#include <gfx/cascade_shadow.h>
#include <gfx/resources.h>
#include <sky/sky_state.h>

#include <memory>

namespace imp::ecs { class World; }
namespace imp::gfx
{
	class ICommandList;
	class IRenderTarget;
}

namespace imp::engine
{
	class DeferredRenderer
	{
	public:
		static constexpr gfx::SampleCount kSampleCount = gfx::SampleCount::One;

		DeferredRenderer();
		~DeferredRenderer();

		DeferredRenderer(const DeferredRenderer&) = delete;
		DeferredRenderer& operator=(const DeferredRenderer&) = delete;

		bool init(RenderContext& ctx, const RendererDesc& desc);
		void shutdown();

		void update(RenderContext& ctx, ecs::World& world, const fwk::Camera& camera, const sky::SkyState& sky);
		void render(RenderContext&, gfx::ICommandList& cmd, const fwk::Camera& camera);

		void addOutput(gfx::IRenderTarget* target);
		void removeOutput(gfx::IRenderTarget* target);
		void clearOutputs();

		[[nodiscard]] RenderFeatures& features();
		[[nodiscard]] const RenderFeatures& features() const;

		[[nodiscard]] gfx::CascadeConfig& cascadeConfig();
		[[nodiscard]] const gfx::CascadeConfig& cascadeConfig() const;

		[[nodiscard]] gfx::TextureFormat hdrColourFormat() const;
		[[nodiscard]] gfx::TextureFormat hdrDepthFormat() const;

		[[nodiscard]] bool initialised() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
