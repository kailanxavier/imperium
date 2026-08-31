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
	class RenderGraphResourcePool;
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

		void ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount);
		bool reloadShaders(AppContext& ctx, const AssetManifest& assets);

		static constexpr gfx::SampleCount kMsaaSampleCount = gfx::SampleCount::Four;

		gfx::IPipeline& meshPipeline() { return *m_pipeline; }
		gfx::IPipeline& blendPipeline() { return *m_blendPipeline; }
		gfx::IPipeline& shadowPipeline() { return *m_shadowPipeline; }
		gfx::IPipeline& skyPipeline() { return *m_skyPipeline; }
		gfx::IPipeline& tonemapPipeline() { return *m_tonemapPipeline; }
		gfx::ISampler& sampler() { return *m_sampler; }
		gfx::ISampler& shadowSampler() { return *m_shadowSampler; }

		[[nodiscard]] gfx::TextureFormat hdrColourFormat() const { return m_hdrColourFormat; }
		[[nodiscard]] gfx::TextureFormat hdrDepthFormat() const { return m_hdrDepthFormat; }

		[[nodiscard]] gfx::IRenderTarget& shadowCascadeTarget(u32 i) const { return *m_shadowCascadeTargets[i]; }
		[[nodiscard]] gfx::ITexture* shadowArrayTexture() const { return m_shadowArrayTexture; }

		[[nodiscard]] gfx::IBuffer& cascadeUBO(u32 frame) const { return *m_cascadeUBOs[frame]; }
		[[nodiscard]] gfx::IBuffer& lightUBO(u32 frame) const { return *m_lightUBOs[frame]; }
		[[nodiscard]] gfx::IBuffer& instanceBuffer(u32 frame) const { return *m_instanceBuffers[frame]; }
		[[nodiscard]] bool hasInstanceBuffers() const { return !m_instanceBuffers.empty(); }

		[[nodiscard]] gfx::RenderGraphResourcePool& graphPool() const { return *m_graphPool; }

	private:
		struct ShaderPipelineSet
		{
			std::unique_ptr<gfx::IShader> meshVertShader, meshFragShader;
			std::unique_ptr<gfx::IShader> shadowVertShader, shadowFragShader;
			std::unique_ptr<gfx::IShader> skyVertShader, skyFragShader;
			std::unique_ptr<gfx::IShader> tonemapVertShader, tonemapFragShader;

			std::unique_ptr<gfx::IPipeline> pipeline;
			std::unique_ptr<gfx::IPipeline> blendPipeline;
			std::unique_ptr<gfx::IPipeline> shadowPipeline;
			std::unique_ptr<gfx::IPipeline> skyPipeline;
			std::unique_ptr<gfx::IPipeline> tonemapPipeline;
		};

		bool buildShaderPipelineSet(AppContext& ctx, const AssetManifest& assets, ShaderPipelineSet& out) const;
		void adoptShaderPipelineSet(ShaderPipelineSet&& set);

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

		gfx::TextureFormat m_hdrColourFormat = gfx::TextureFormat::RGBA16Float;
		gfx::TextureFormat m_hdrDepthFormat = gfx::TextureFormat::Depth32Float;

		std::vector<std::unique_ptr<gfx::IRenderTarget>> m_shadowCascadeTargets;
		gfx::ITexture* m_shadowArrayTexture = nullptr;

		std::vector<std::unique_ptr<gfx::IBuffer>> m_cascadeUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_lightUBOs;
		std::vector<std::unique_ptr<gfx::IBuffer>> m_instanceBuffers;
		u32 m_instanceCapacity = 16;

		std::unique_ptr<gfx::RenderGraphResourcePool> m_graphPool;
	};
}
