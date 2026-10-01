#pragma once

#include <engine/render_context.h>

#include <camera/camera.h>
#include <gfx/cascade_shadow.h>
#include <gfx/model_registry.h>
#include <gfx/render_extraction.h>
#include <sky/sky_state.h>

#include <array>
#include <memory>

namespace imp::ecs { class World; }
namespace imp::gfx
{
	class ITlas;
	class IBuffer;
}

namespace imp::engine
{
	class RenderScene
	{
	public:
		RenderScene() = default;
		~RenderScene() = default;

		RenderScene(const RenderScene&) = delete;
		RenderScene& operator=(const RenderScene&) = delete;

		void init(gfx::ModelRegistry& registry);
		void shutdown();

		void update(RenderContext& ctx, ecs::World& world, const fwk::Camera& camera,
			const sky::SkyState& sky, bool buildTlas);

		void recomputeCascades(const fwk::Camera& camera, float aspect);
		u32 instanceCount() const { return static_cast<u32>( m_extraction.instanceData.size() ); }

		const gfx::RenderExtraction& extraction() const { return m_extraction; }
		const std::array<gfx::CascadeData, gfx::kCascadeCount>& cascades() const { return m_cascades; }

		const gfx::CascadeConfig& cascadeConfig() const { return m_cascadeConfig; }
		gfx::CascadeConfig& cascadeConfig() { return m_cascadeConfig; }

		const gfx::ITlas* staticTlas() const { return m_staticTlas.get(); }
		gfx::IBuffer* ddgiInstanceMaterials() const { return m_ddgiInstanceMaterials.get(); }

		gfx::ModelRegistry& modelRegistry() { return *m_modelRegistry; }

	private:
		void updateSunViewProj();
		void updateDynamicTlas(RenderContext& ctx);
		void releaseTlas(RenderContext& ctx);

		gfx::ModelRegistry* m_modelRegistry = nullptr;
		math::Vec3f m_mainLightDirection = math::Vec3f{ 0.f, -1.f, 0.f };
		math::Mat4f m_sunViewProj = math::Mat4f::identity();

		gfx::CascadeConfig m_cascadeConfig;
		std::array<gfx::CascadeData, gfx::kCascadeCount> m_cascades{};

		gfx::RenderExtraction m_extraction;
		std::shared_ptr<gfx::ITlas> m_staticTlas;
		std::shared_ptr<gfx::IBuffer> m_ddgiInstanceMaterials;
	};
}
