#pragma once

#include <app/iapp.h>
#include <camera/camera.h>

#include <gfx/model.h>
#include <gfx/model_registry.h>

#include <core/types/int_types.h>
#include <core/math/math.h>

#include <ecs/world.h>

#include <gfx/render_extraction.h>
#include <gfx/texture_cache.h>
#include <gfx/cascade_shadow.h>

#include <memory>

namespace imp::gfx
{
	class IShader;
	class IPipeline;
	class ISampler;
	class IBuffer;
}

namespace imp::app
{
	class SandboxApp final : public IApp
	{
	public:
		bool onInit(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;
		void onRender(AppContext& ctx, gfx::ICommandList& cmd) override;
		void onShutdown(AppContext& ctx) override;

		math::Vec3f& sunDirection() { return m_sunDirection; }
		const math::Vec3f& sunDirection() const { return m_sunDirection; }

		ecs::Transform& pointPos() { return m_localLightTransform; }
		const ecs::Transform& pointPos() const { return m_localLightTransform; }

		const fwk::Camera& camera() const { return m_camera; }
		gfx::TextureFormat hdrColourFormat() const { return m_hdrTarget->format(); }
		gfx::TextureFormat hdrDepthFormat() const { return m_hdrDepthTarget->format(); }
		gfx::SampleCount sampleCount() const { return kMsaaSampleCount; }

	private:
		void drawNode(gfx::ICommandList& cmd, gfx::Model& model, const math::Mat4f& viewProj, u32 nodeIdx, const math::Mat4f& parentWorld);

		void ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount);
		void ensureHdrTargetSize(AppContext& ctx);

		ecs::EntityId spawnInstance(AppContext& ctx, const ecs::Transform& t);

	private:
		void updateSunViewProj();
		static constexpr u32 kShadowMapSize = 8192;

		std::unique_ptr<gfx::IShader> m_shadowVertShader;
		std::unique_ptr<gfx::IShader> m_shadowFragShader;
		std::unique_ptr<gfx::IPipeline> m_shadowPipeline;
		std::unique_ptr<gfx::IRenderTarget> m_shadowTarget;
		std::unique_ptr<gfx::ISampler> m_shadowSampler;

		math::Vec3f m_sunDirection = math::Vec3f::zero();
		math::Mat4f m_sunViewProj = math::Mat4f::identity();

		ecs::Transform m_localLightTransform{};
		ecs::EntityId m_localLight{};

	private:
		fwk::Camera m_camera;

		std::unique_ptr<gfx::IShader> m_meshVertShader;
		std::unique_ptr<gfx::IShader> m_meshFragShader;

		std::unique_ptr<gfx::IPipeline> m_pipeline;
		std::unique_ptr<gfx::IPipeline> m_blendPipeline;

		std::unique_ptr<gfx::ISampler> m_sampler;
		//std::unique_ptr<gfx::IBuffer> m_instanceBuffer;

		static constexpr gfx::SampleCount kMsaaSampleCount = gfx::SampleCount::Four;

		std::unique_ptr<gfx::IRenderTarget> m_hdrTarget;
		std::unique_ptr<gfx::IRenderTarget> m_hdrDepthTarget;
		std::unique_ptr<gfx::IRenderTarget> m_hdrResolveTarget;

		std::unique_ptr<gfx::IShader> m_tonemapVertShader;
		std::unique_ptr<gfx::IShader> m_tonemapFragShader;
		std::unique_ptr<gfx::IPipeline> m_tonemapPipeline;

		std::unique_ptr<gfx::IShader> m_skyVertShader;
		std::unique_ptr<gfx::IShader> m_skyFragShader;
		std::unique_ptr<gfx::IPipeline> m_skyPipeline;

		ecs::EntityId m_sunEntity;

		u32 m_instanceCapacity = 16;

		std::vector<ecs::EntityId> m_instances;
		gfx::ModelRegistry m_modelRegistry;

		ecs::ModelHandle m_environmentHandle{};
		ecs::ModelHandle m_statueHandle{};

		gfx::RenderExtraction m_extraction;

		std::vector<std::unique_ptr<gfx::IRenderTarget>> m_shadowCascadeTargets;
		gfx::ITexture* m_shadowArrayTexture = nullptr;
		std::array<gfx::CascadeData, gfx::kCascadeCount> m_cascades;
		gfx::CascadeConfig m_cascadeConfig;

		std::vector<std::unique_ptr<gfx::IBuffer>> m_cascadeUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_lightUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_instanceBuffers;
	};
}
