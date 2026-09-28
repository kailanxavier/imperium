#pragma once
#include <app/iapp.h>
#include <sandbox/asset_manifest.h>
#include <camera/camera.h>
#include <ecs/world.h>
#include <gfx/model_registry.h>
#include <gfx/render_extraction.h>
#include <gfx/cascade_shadow.h>
#include <engine/render_scene.h>
#include <array>
#include <vector>
#include <gfx/texture_cache.h>

namespace imp::app
{
	class SandboxScene final : public engine::IRenderScene
	{
	public:
		SandboxScene();
		~SandboxScene();

		bool init(AppContext& ctx, const AssetManifest& assets);
		void update(AppContext& ctx, const fwk::Camera& camera);
		void shutdown(AppContext& ctx);

		ecs::EntityId spawnInstance(AppContext& ctx, const ecs::Transform& t, const gfx::ModelHandle& model);

		u32 instanceCount() const { return static_cast<u32>( m_extraction.instanceData.size() ); }

		const gfx::RenderExtraction& extraction() const override { return m_extraction; }
		const std::array<gfx::CascadeData, gfx::kCascadeCount>& cascades() const override { return m_cascades; }
		const gfx::CascadeConfig& cascadeConfig() const override { return m_cascadeConfig; }
		const gfx::ITlas* staticTlas() const override { return m_staticTlas.get(); }
		gfx::IBuffer* ddgiInstanceMaterials() const override { return m_ddgiInstanceMaterials.get(); }

		gfx::ModelRegistry& modelRegistry() override { return m_modelRegistry; }

		void recomputeCascades(const fwk::Camera& camera, float aspect);

		math::Vec3f& sunDirection() { return m_sunDirection; }
		const math::Vec3f& sunDirection() const override { return m_sunDirection; }
		const math::Mat4f& sunViewProj() const { return m_sunViewProj; }

		gfx::CascadeConfig& cascadeConfig() { return m_cascadeConfig; }

		ecs::Transform& pointLightTransform() { return m_localLightTransform; }
		const ecs::Transform& pointLightTransform() const { return m_localLightTransform; }

	private:
		void syncSunTransform(AppContext& ctx) const;
		void updateSunViewProj();
		void updateDynamicTlas(AppContext& ctx);

		gfx::ModelRegistry m_modelRegistry;
		ecs::ModelHandle m_environmentHandle{};
		ecs::ModelHandle m_environmentTestHandle{};

		ecs::EntityId m_sunEntity{};
		ecs::EntityId m_localLight{};
		std::vector<ecs::EntityId> m_instances;

		math::Vec3f m_sunDirection = math::Vec3f::zero();
		math::Mat4f m_sunViewProj = math::Mat4f::identity();
		ecs::Transform m_localLightTransform{};

		gfx::CascadeConfig m_cascadeConfig;
		std::array<gfx::CascadeData, gfx::kCascadeCount> m_cascades{};

		gfx::RenderExtraction m_extraction;
		std::shared_ptr<gfx::ITlas> m_staticTlas;
		//bool m_staticTlasBuildAttempted = false;

		std::shared_ptr<gfx::IBuffer> m_ddgiInstanceMaterials;
	};
}
