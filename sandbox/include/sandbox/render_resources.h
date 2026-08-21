#pragma once
#include <app/iapp.h>
#include <sandbox/asset_manifest.h>
#include <gfx/cascade_shadow.h>
#include <core/types/int_types.h>
#include <memory>
#include <vector>

namespace imp::gfx
{
	class IShader;
	class IPipeline;
	class ISampler;
	class IBuffer;
	class IRenderTarget;
	class ITexture;
}

namespace imp::app
{
	class RenderResources
	{
	public:
		RenderResources();
		~RenderResources();

		bool init(AppContext& ctx, const AssetManifest& assets, const gfx::CascadeConfig& cascadeConfig);
		void shutdown();

		void ensureHdrTargetSize(AppContext& ctx);
		void ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount);

		static constexpr gfx::SampleCount kMsaaSampleCount = gfx::SampleCount::Four;

		gfx::IPipeline& meshPipeline() { return *m_pipeline; }
		gfx::IPipeline& blendPipeline() { return *m_blendPipeline; }
		gfx::IPipeline& shadowPipeline() { return *m_shadowPipeline; }
		gfx::IPipeline& skyPipeline() { return *m_skyPipeline; }
		gfx::IPipeline& tonemapPipeline() { return *m_tonemapPipeline; }
		gfx::ISampler& sampler() { return *m_sampler; }
		gfx::ISampler& shadowSampler() { return *m_shadowSampler; }

		gfx::IRenderTarget& hdrTarget() { return *m_hdrTarget; }
		gfx::IRenderTarget& hdrDepthTarget() { return *m_hdrDepthTarget; }
		gfx::IRenderTarget& hdrResolveTarget() { return *m_hdrResolveTarget; }
		gfx::TextureFormat hdrColourFormat() const { return m_hdrTarget->format(); }
		gfx::TextureFormat hdrDepthFormat() const { return m_hdrDepthTarget->format(); }

		gfx::IRenderTarget& shadowCascadeTarget(u32 i) { return *m_shadowCascadeTargets[i]; }
		gfx::ITexture* shadowArrayTexture() { return m_shadowArrayTexture; }

		gfx::IBuffer& cascadeUBO(u32 frame) { return *m_cascadeUBOs[frame]; }
		gfx::IBuffer& lightUBO(u32 frame) { return *m_lightUBOs[frame]; }
		gfx::IBuffer& instanceBuffer(u32 frame) { return *m_instanceBuffers[frame]; }
		bool hasInstanceBuffers() const { return !m_instanceBuffers.empty(); }

	private:
		std::unique_ptr<gfx::IShader> m_meshVertShader, m_meshFragShader;
		std::unique_ptr<gfx::IShader> m_shadowVertShader, m_shadowFragShader;
		std::unique_ptr<gfx::IShader> m_tonemapVertShader, m_tonemapFragShader;
		std::unique_ptr<gfx::IShader> m_skyVertShader, m_skyFragShader;

		std::unique_ptr<gfx::IPipeline> m_pipeline;
		std::unique_ptr<gfx::IPipeline> m_blendPipeline;
		std::unique_ptr<gfx::IPipeline> m_shadowPipeline;
		std::unique_ptr<gfx::IPipeline> m_skyPipeline;
		std::unique_ptr<gfx::IPipeline> m_tonemapPipeline;

		std::unique_ptr<gfx::ISampler> m_sampler;
		std::unique_ptr<gfx::ISampler> m_shadowSampler;

		std::unique_ptr<gfx::IRenderTarget> m_hdrTarget;
		std::unique_ptr<gfx::IRenderTarget> m_hdrDepthTarget;
		std::unique_ptr<gfx::IRenderTarget> m_hdrResolveTarget;

		std::vector<std::unique_ptr<gfx::IRenderTarget>> m_shadowCascadeTargets;
		gfx::ITexture* m_shadowArrayTexture = nullptr;

		std::vector<std::unique_ptr<gfx::IBuffer>> m_cascadeUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_lightUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_instanceBuffers;
		u32 m_instanceCapacity = 16;
	};
}
