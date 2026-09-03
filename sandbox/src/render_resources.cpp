#include <sandbox/render_resources.h>
#include <gfx/model.h>
#include <gfx/lighting.h>
#include <gfx/config.h>
#include <gfx/render_graph_resource_pool.h>
#include <core/log/log.h>
#include <cstddef>
#include <algorithm>
#include <core/config/cvar.h>
#include <gfx/ao.h>

namespace imp::app
{
	RenderResources::RenderResources() = default;
	RenderResources::~RenderResources() = default;

	bool RenderResources::buildShaderPipelineSet(AppContext &ctx, const AssetManifest &assets, ShaderPipelineSet &out) const
	{
		gfx::ShaderDesc meshVertDesc;
		meshVertDesc.stage = gfx::ShaderStage::Vertex;
		meshVertDesc.path = assets.meshVertShader;
		out.meshVertShader = ctx.gfx.createShader(meshVertDesc);

		gfx::ShaderDesc meshFragDesc;
		meshFragDesc.stage = gfx::ShaderStage::Fragment;
		meshFragDesc.path = assets.meshFragShader;
		out.meshFragShader = ctx.gfx.createShader(meshFragDesc);

		if (!out.meshFragShader || !out.meshVertShader)
		{
			LOG_ERROR("Sandbox", "Failed to load mesh shaders.");
			return false;
		}

		gfx::ShaderDesc tonemapVertDesc;
		tonemapVertDesc.stage = gfx::ShaderStage::Vertex;
		tonemapVertDesc.path = assets.tonemapVertShader;
		out.tonemapVertShader = ctx.gfx.createShader(tonemapVertDesc);

		gfx::ShaderDesc tonemapFragDesc;
		tonemapFragDesc.stage = gfx::ShaderStage::Fragment;
		tonemapFragDesc.path = assets.tonemapFragShader;
		out.tonemapFragShader = ctx.gfx.createShader(tonemapFragDesc);

		if (!out.tonemapVertShader || !out.tonemapFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load tonemap shaders.");
			return false;
		}

		gfx::ShaderDesc shadowVertDesc;
		shadowVertDesc.stage = gfx::ShaderStage::Vertex;
		shadowVertDesc.path = assets.shadowVertShader;
		out.shadowVertShader = ctx.gfx.createShader(shadowVertDesc);

		gfx::ShaderDesc shadowFragDesc;
		shadowFragDesc.stage = gfx::ShaderStage::Fragment;
		shadowFragDesc.path = assets.shadowFragShader;
		out.shadowFragShader = ctx.gfx.createShader(shadowFragDesc);

		if (!out.shadowVertShader || !out.shadowFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load shadow shaders.");
			return false;
		}

		gfx::ShaderDesc skyVertDesc;
		skyVertDesc.stage = gfx::ShaderStage::Vertex;
		skyVertDesc.path = assets.skyVertShader;
		out.skyVertShader = ctx.gfx.createShader(skyVertDesc);

		gfx::ShaderDesc skyFragDesc;
		skyFragDesc.stage = gfx::ShaderStage::Fragment;
		skyFragDesc.path = assets.skyFragShader;
		out.skyFragShader = ctx.gfx.createShader(skyFragDesc);

		if (!out.skyFragShader || !out.skyVertShader)
		{
			LOG_ERROR("Sandbox", "Failed to load sky shaders");
			return false;
		}

		gfx::ShaderDesc prepassVertDesc;
		prepassVertDesc.stage = gfx::ShaderStage::Vertex;
		prepassVertDesc.path = assets.prepassVertShader;
		out.prepassVertShader = ctx.gfx.createShader(prepassVertDesc);

		gfx::ShaderDesc prepassFragDesc;
		prepassFragDesc.stage = gfx::ShaderStage::Fragment;
		prepassFragDesc.path = assets.prepassFragShader;
		out.prepassFragShader = ctx.gfx.createShader(prepassFragDesc);

		if (!out.prepassVertShader || !out.prepassFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load depth/normal prepass shaders.");
			return false;
		}

		gfx::ShaderDesc fullscreenVertDesc;
		fullscreenVertDesc.stage = gfx::ShaderStage::Vertex;
		fullscreenVertDesc.path = assets.fullscreenVertShader;
		out.fullscreenVertShader = ctx.gfx.createShader(fullscreenVertDesc);

		gfx::ShaderDesc gtaoFragDesc;
		gtaoFragDesc.stage = gfx::ShaderStage::Fragment;
		gtaoFragDesc.path = assets.gtaoFragShader;
		out.gtaoFragShader = ctx.gfx.createShader(gtaoFragDesc);

		gfx::ShaderDesc blurFragDesc;
		blurFragDesc.stage = gfx::ShaderStage::Fragment;
		blurFragDesc.path = assets.blurFragShader;
		out.blurFragShader = ctx.gfx.createShader(blurFragDesc);

		if (!out.fullscreenVertShader || !out.gtaoFragShader || !out.blurFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load AO shaders.");
			return false;
		}

		gfx::VertexAttribute meshAttrs[4] = {
			{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
			{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
			{ 2, static_cast<u32>( offsetof(gfx::ModelVertex, uv) ), 2, true },
			{ 3, static_cast<u32>( offsetof(gfx::ModelVertex, tangent) ), 4, true},
		};

		gfx::VertexAttribute shadowAttrs[3] = {
			{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
			{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
			{ 2, static_cast<u32>( offsetof(gfx::ModelVertex, uv) ), 2, true },
		};

		gfx::VertexAttribute instanceAttrs[4] = {
			{ 4, static_cast<u32>( sizeof(math::Vec4f) * 0 ), 4, true },
			{ 5, static_cast<u32>( sizeof(math::Vec4f) * 1 ), 4, true },
			{ 6, static_cast<u32>( sizeof(math::Vec4f) * 2 ), 4, true },
			{ 7, static_cast<u32>( sizeof(math::Vec4f) * 3 ), 4, true },
		};

		gfx::PipelineDesc skyPipelineDesc{};
		skyPipelineDesc.vertexShader = out.skyVertShader.get();
		skyPipelineDesc.fragmentShader = out.skyFragShader.get();
		skyPipelineDesc.rasterizerState.cullMode = gfx::CullMode::None;
		skyPipelineDesc.depthStencilState.depthTestEnable = true;
		skyPipelineDesc.depthStencilState.depthWriteEnable = false;
		skyPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::LessOrEqual;
		skyPipelineDesc.blendState.blendEnable = false;
		skyPipelineDesc.colourFormat = m_hdrColourFormat;
		skyPipelineDesc.depthFormat = m_hdrDepthFormat;
		skyPipelineDesc.sampleCount = kMsaaSampleCount;
		skyPipelineDesc.hasInstanceBinding = false;
		out.skyPipeline = ctx.gfx.createPipeline(skyPipelineDesc);

		if (!out.skyPipeline)
		{
			LOG_ERROR("Sandbox", "Failed to create sky pipeline");
			return false;
		}

		gfx::PipelineDesc meshPipelineDesc{};
		meshPipelineDesc.vertexShader = out.meshVertShader.get();
		meshPipelineDesc.fragmentShader = out.meshFragShader.get();
		meshPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
		meshPipelineDesc.vertexLayout.attributeCount = 4;
		meshPipelineDesc.vertexLayout.attributes = meshAttrs;
		meshPipelineDesc.instanceLayout.stride = sizeof(math::Mat4f);
		meshPipelineDesc.instanceLayout.attributeCount = 4;
		meshPipelineDesc.instanceLayout.attributes = instanceAttrs;
		meshPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
		meshPipelineDesc.depthStencilState.depthTestEnable = true;
		meshPipelineDesc.depthStencilState.depthWriteEnable = true;
		meshPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		meshPipelineDesc.blendState.blendEnable = false;
		meshPipelineDesc.colourFormat = m_hdrColourFormat;
		meshPipelineDesc.depthFormat = m_hdrDepthFormat;
		meshPipelineDesc.sampleCount = kMsaaSampleCount;
		meshPipelineDesc.hasInstanceBinding = true;
		out.pipeline = ctx.gfx.createPipeline(meshPipelineDesc);

		gfx::PipelineDesc blendPipelineDesc{ meshPipelineDesc };
		blendPipelineDesc.blendState.blendEnable = true;
		blendPipelineDesc.depthStencilState.depthTestEnable = true;
		blendPipelineDesc.depthStencilState.depthWriteEnable = false;
		blendPipelineDesc.colourFormat = m_hdrColourFormat;
		out.blendPipeline = ctx.gfx.createPipeline(blendPipelineDesc);

		gfx::PipelineDesc tonemapPipelineDesc;
		tonemapPipelineDesc.vertexShader = out.tonemapVertShader.get();
		tonemapPipelineDesc.fragmentShader = out.tonemapFragShader.get();
		tonemapPipelineDesc.colourFormat = ctx.gfx.backBuffer().format();
		tonemapPipelineDesc.depthFormat = gfx::TextureFormat::Unknown;
		out.tonemapPipeline = ctx.gfx.createPipeline(tonemapPipelineDesc);

		gfx::PipelineDesc shadowPipelineDesc{};
		shadowPipelineDesc.vertexShader = out.shadowVertShader.get();
		shadowPipelineDesc.fragmentShader = out.shadowFragShader.get();
		shadowPipelineDesc.vertexLayout.attributeCount = 3;
		shadowPipelineDesc.vertexLayout.attributes = shadowAttrs;
		shadowPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
		shadowPipelineDesc.instanceLayout = meshPipelineDesc.instanceLayout;
		shadowPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back; // reduces acne on closed meshes
		shadowPipelineDesc.depthStencilState.depthTestEnable = true;
		shadowPipelineDesc.depthStencilState.depthWriteEnable = true;
		shadowPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		shadowPipelineDesc.colourFormat = gfx::TextureFormat::Unknown;
		shadowPipelineDesc.depthFormat = gfx::TextureFormat::Depth32Float;
		shadowPipelineDesc.hasInstanceBinding = true;
		out.shadowPipeline = ctx.gfx.createPipeline(shadowPipelineDesc);

		if (!out.pipeline || !out.blendPipeline || !out.tonemapPipeline || !out.shadowPipeline)
		{
			LOG_ERROR("Sandbox", "Failed to create one or more pipelines");
			return false;
		}

		gfx::VertexAttribute prepassAttrs[3] = {
			{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
			{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
			{ 2, static_cast<u32>(offsetof(gfx::ModelVertex, uv)), 2, true },
		};

		gfx::PipelineDesc prepassPipelineDesc{};
		prepassPipelineDesc.vertexShader = out.prepassVertShader.get();
		prepassPipelineDesc.fragmentShader = out.prepassFragShader.get();
		prepassPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
		prepassPipelineDesc.vertexLayout.attributeCount = 3;
		prepassPipelineDesc.vertexLayout.attributes = prepassAttrs;
		prepassPipelineDesc.instanceLayout = meshPipelineDesc.instanceLayout;
		prepassPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
		prepassPipelineDesc.depthStencilState.depthTestEnable = true;
		prepassPipelineDesc.depthStencilState.depthWriteEnable = true;
		prepassPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		prepassPipelineDesc.blendState.blendEnable = false;
		prepassPipelineDesc.colourFormat = gfx::TextureFormat::RGBA16Float;
		prepassPipelineDesc.colourFormat1 = gfx::TextureFormat::RGBA8Unorm;
		prepassPipelineDesc.depthFormat = gfx::TextureFormat::Depth32Float;
		prepassPipelineDesc.sampleCount = gfx::SampleCount::One;
		prepassPipelineDesc.hasInstanceBinding = true;
		out.prepassPipeline = ctx.gfx.createPipeline(prepassPipelineDesc);

		gfx::PipelineDesc gtaoPipelineDesc{};
		gtaoPipelineDesc.vertexShader = out.fullscreenVertShader.get();
		gtaoPipelineDesc.fragmentShader = out.gtaoFragShader.get();
		gtaoPipelineDesc.colourFormat = gfx::TextureFormat::RGBA8Unorm;
		gtaoPipelineDesc.depthFormat = gfx::TextureFormat::Unknown;
		gtaoPipelineDesc.sampleCount = gfx::SampleCount::One;
		gtaoPipelineDesc.hasInstanceBinding = false;
		out.gtaoPipeline = ctx.gfx.createPipeline(gtaoPipelineDesc);

		gfx::PipelineDesc blurPipelineDesc{ gtaoPipelineDesc };
		blurPipelineDesc.fragmentShader = out.blurFragShader.get();
		out.blurPipeline = ctx.gfx.createPipeline(blurPipelineDesc);

		if (!out.prepassPipeline || !out.gtaoPipeline || !out.blurPipeline)
		{
			LOG_ERROR("Sandbox", "Failed to create one or more AO pipelines");
			return false;
		}

		return true;
	}

	void RenderResources::adoptShaderPipelineSet(ShaderPipelineSet&& set)
	{
		m_meshVertShader = std::move(set.meshVertShader);
		m_meshFragShader = std::move(set.meshFragShader);
		m_shadowVertShader = std::move(set.shadowVertShader);
		m_shadowFragShader = std::move(set.shadowFragShader);
		m_tonemapVertShader = std::move(set.tonemapVertShader);
		m_tonemapFragShader = std::move(set.tonemapFragShader);
		m_skyVertShader = std::move(set.skyVertShader);
		m_skyFragShader = std::move(set.skyFragShader);
		m_prepassVertShader = std::move(set.prepassVertShader);
		m_prepassFragShader = std::move(set.prepassFragShader);
		m_fullscreenVertShader = std::move(set.fullscreenVertShader);
		m_gtaoFragShader = std::move(set.gtaoFragShader);
		m_blurFragShader = std::move(set.blurFragShader);

		m_pipeline = std::move(set.pipeline);
		m_blendPipeline = std::move(set.blendPipeline);
		m_shadowPipeline = std::move(set.shadowPipeline);
		m_skyPipeline = std::move(set.skyPipeline);
		m_tonemapPipeline = std::move(set.tonemapPipeline);
		m_prepassPipeline = std::move(set.prepassPipeline);
		m_gtaoPipeline = std::move(set.gtaoPipeline);
		m_blurPipeline = std::move(set.blurPipeline);
	}

	bool RenderResources::reloadShaders(AppContext &ctx, const AssetManifest &assets)
	{
		ShaderPipelineSet fresh;
		if (!buildShaderPipelineSet(ctx, assets, fresh))
		{
			LOG_ERROR("Sandbox", "Shader hot reload failed, keeping the last working pipelines");
			return false;
		}

		ctx.gfx.waitIdle();
		adoptShaderPipelineSet(std::move(fresh));

		LOG_INFO("Sandbox", "Shader hot reload succeeded");
		return true;
	}



	bool RenderResources::init(AppContext& ctx, const AssetManifest& assets, const gfx::CascadeConfig& cascadeConfig)
	{
		m_graphPool = std::make_unique<gfx::RenderGraphResourcePool>(ctx.gfx);

		ShaderPipelineSet initial;
		if (!buildShaderPipelineSet(ctx, assets, initial))
			return false;
		adoptShaderPipelineSet(std::move(initial));

		gfx::SamplerDesc samplerDesc{};
		samplerDesc.minFilter = gfx::FilterMode::Linear;
		samplerDesc.magFilter = gfx::FilterMode::Linear;
		samplerDesc.addressModeU = gfx::AddressMode::Repeat;
		samplerDesc.addressModeV = gfx::AddressMode::Repeat;
		samplerDesc.enableAnisotropy = true;
		m_sampler = ctx.gfx.createSampler(samplerDesc);

		gfx::TextureDesc cascadeDesc;
		cascadeDesc.width = cascadeConfig.shadowMapResolution;
		cascadeDesc.height = cascadeConfig.shadowMapResolution;
		cascadeDesc.arrayLayers = gfx::kCascadeCount;
		cascadeDesc.format = gfx::TextureFormat::Depth32Float;
		cascadeDesc.usage = gfx::TextureUsage::DepthStencil | gfx::TextureUsage::Sampled;
		m_shadowCascadeTargets = ctx.gfx.createCascadeRenderTargets(cascadeDesc, &m_shadowArrayTexture);
		if (m_shadowCascadeTargets.size() != gfx::kCascadeCount)
		{
			LOG_ERROR("Sandbox", "createCascadeRenderTargets() returned {} targets, expected {}",
				m_shadowCascadeTargets.size(), gfx::kCascadeCount);
		}

		gfx::SamplerDesc shadowSamplerDesc{};
		shadowSamplerDesc.minFilter = gfx::FilterMode::Linear;
		shadowSamplerDesc.magFilter = gfx::FilterMode::Linear;
		shadowSamplerDesc.addressModeU = gfx::AddressMode::ClampToEdge;
		shadowSamplerDesc.addressModeV = gfx::AddressMode::ClampToEdge;
		m_shadowSampler = ctx.gfx.createSampler(shadowSamplerDesc);

		gfx::BufferDesc cascadeUboDesc{};
		cascadeUboDesc.size = sizeof(gfx::CascadeUBO);
		cascadeUboDesc.usage = gfx::BufferUsage::Uniform;
		cascadeUboDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_cascadeUBOs.resize(gfx::kMaxFramesInFlight);
		for (auto& buf : m_cascadeUBOs)
			buf = ctx.gfx.createBuffer(cascadeUboDesc);

		gfx::BufferDesc lightUboDesc{};
		lightUboDesc.size = sizeof(gfx::LightUBO);
		lightUboDesc.usage = gfx::BufferUsage::Uniform;
		lightUboDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_lightUBOs.resize(gfx::kMaxFramesInFlight);
		for (auto& buf : m_lightUBOs)
			buf = ctx.gfx.createBuffer(lightUboDesc);

		gfx::BufferDesc aoParamsDesc{};
		aoParamsDesc.size = sizeof(gfx::AOParamsUBO);
		aoParamsDesc.usage = gfx::BufferUsage::Uniform;
		aoParamsDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_aoParamsUBOs.resize(gfx::kMaxFramesInFlight);
		for (auto& buf : m_aoParamsUBOs)
			buf = ctx.gfx.createBuffer(aoParamsDesc);

		gfx::BufferDesc screenParamsDesc{};
		screenParamsDesc.size = sizeof(gfx::ScreenParamsUBO);
		screenParamsDesc.usage = gfx::BufferUsage::Uniform;
		screenParamsDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_screenParamsUBOs.resize(gfx::kMaxFramesInFlight);
		for (auto& buf : m_screenParamsUBOs)
			buf = ctx.gfx.createBuffer(screenParamsDesc);

		gfx::BufferDesc blurParamsDesc{};
		blurParamsDesc.size = sizeof(gfx::BlurParamsUBO);
		blurParamsDesc.usage = gfx::BufferUsage::Uniform;
		blurParamsDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_blurParamsUBOs.resize(gfx::kMaxFramesInFlight);
		for (auto& buf : m_blurParamsUBOs)
			buf = ctx.gfx.createBuffer(blurParamsDesc);

		if (!m_pipeline || !m_blendPipeline || !m_tonemapPipeline || !m_sampler
			|| !m_shadowPipeline || !m_shadowSampler)
		{
			LOG_FATAL("Sandbox", "Failed to create pipelines/sampler/light buffer");
			return false;
		}

		return true;
	}

	void RenderResources::shutdown()
	{
		m_sampler.reset();
		m_pipeline.reset();
		m_tonemapPipeline.reset();
		m_blendPipeline.reset();
		m_tonemapFragShader.reset();
		m_tonemapVertShader.reset();
		m_meshFragShader.reset();
		m_meshVertShader.reset();
		m_shadowPipeline.reset();
		m_shadowSampler.reset();
		m_shadowFragShader.reset();
		m_shadowVertShader.reset();
		m_shadowCascadeTargets.clear();
		m_skyFragShader.reset();
		m_skyVertShader.reset();
		m_skyPipeline.reset();
		m_prepassPipeline.reset();
		m_gtaoPipeline.reset();
		m_blurPipeline.reset();
		m_prepassVertShader.reset();
		m_prepassFragShader.reset();
		m_fullscreenVertShader.reset();
		m_gtaoFragShader.reset();
		m_blurFragShader.reset();

		for (auto& buf : m_cascadeUBOs) buf.reset();
		for (auto& buf : m_lightUBOs) buf.reset();
		for (auto& buf : m_instanceBuffers) buf.reset();
		for (auto& buf : m_aoParamsUBOs) buf.reset();
		for (auto& buf : m_screenParamsUBOs) buf.reset();
		for (auto& buf : m_blurParamsUBOs) buf.reset();

		m_graphPool.reset();
	}

	void RenderResources::ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount)
	{
		if (!m_instanceBuffers.empty() && instanceCount <= m_instanceCapacity)
			return;

		static CVarInt cvarMinInstanceCapacity{ "render.min_instance_capacity", 16 };
		static CVarFloat cvarGrowthFactor{ "render.instance_buffer_growth_factor", 2.f };

		u32 newCapacity = std::max<u32>(instanceCount, static_cast<u32>( m_instanceCapacity * static_cast<float>( cvarGrowthFactor ) ));
		newCapacity = std::max<u32>(newCapacity, static_cast<u32>( cvarMinInstanceCapacity ));

		gfx::BufferDesc desc;
		desc.size = static_cast<u64>( newCapacity ) * sizeof(math::Mat4f);
		desc.usage = gfx::BufferUsage::Vertex;
		desc.memoryAccess = gfx::MemoryAccess::HostVisible;
		desc.debugName = "SandboxApp instance buffer";

		std::vector<std::unique_ptr<gfx::IBuffer>> newBuffers(gfx::kMaxFramesInFlight);
		for (auto& buf : newBuffers)
		{
			buf = ctx.gfx.createBuffer(desc);
			if (!buf)
			{
				LOG_ERROR("Sandbox", "Failed to create instance buffer for capacity {}", newCapacity);
				return;
			}
		}

		ctx.gfx.waitIdle();
		m_instanceBuffers = std::move(newBuffers);
		m_instanceCapacity = newCapacity;
	}
}
