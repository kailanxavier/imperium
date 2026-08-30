#include <sandbox/render_resources.h>
#include <gfx/model.h>
#include <gfx/lighting.h>
#include <gfx/config.h>
#include <gfx/render_graph_resource_pool.h>
#include <core/log/log.h>
#include <cstddef>
#include <algorithm>
#include <core/config/cvar.h>

namespace imp::app
{
	RenderResources::RenderResources() = default;
	RenderResources::~RenderResources() = default;

	bool RenderResources::init(AppContext& ctx, const AssetManifest& assets, const gfx::CascadeConfig& cascadeConfig)
	{
		m_graphPool = std::make_unique<gfx::RenderGraphResourcePool>(ctx.gfx);

		gfx::ShaderDesc meshVertDesc;
		meshVertDesc.stage = gfx::ShaderStage::Vertex;
		meshVertDesc.path = assets.meshVertShader;
		m_meshVertShader = ctx.gfx.createShader(meshVertDesc);

		gfx::ShaderDesc meshFragDesc;
		meshFragDesc.stage = gfx::ShaderStage::Fragment;
		meshFragDesc.path = assets.meshFragShader;
		m_meshFragShader = ctx.gfx.createShader(meshFragDesc);

		if (!m_meshFragShader || !m_meshVertShader)
		{
			LOG_ERROR("Sandbox", "Failed to load mesh shaders.");
			return false;
		}

		gfx::ShaderDesc tonemapVertDesc;
		tonemapVertDesc.stage = gfx::ShaderStage::Vertex;
		tonemapVertDesc.path = assets.tonemapVertShader;
		m_tonemapVertShader = ctx.gfx.createShader(tonemapVertDesc);

		gfx::ShaderDesc tonemapFragDesc;
		tonemapFragDesc.stage = gfx::ShaderStage::Fragment;
		tonemapFragDesc.path = assets.tonemapFragShader;
		m_tonemapFragShader = ctx.gfx.createShader(tonemapFragDesc);

		if (!m_tonemapVertShader || !m_tonemapFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load tonemap shaders.");
			return false;
		}

		gfx::ShaderDesc shadowVertDesc;
		shadowVertDesc.stage = gfx::ShaderStage::Vertex;
		shadowVertDesc.path = assets.shadowVertShader;
		m_shadowVertShader = ctx.gfx.createShader(shadowVertDesc);

		gfx::ShaderDesc shadowFragDesc;
		shadowFragDesc.stage = gfx::ShaderStage::Fragment;
		shadowFragDesc.path = assets.shadowFragShader;
		m_shadowFragShader = ctx.gfx.createShader(shadowFragDesc);

		if (!m_shadowVertShader || !m_shadowFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load shadow shaders.");
			return false;
		}

		gfx::ShaderDesc skyVertDesc;
		skyVertDesc.stage = gfx::ShaderStage::Vertex;
		skyVertDesc.path = assets.skyVertShader;
		m_skyVertShader = ctx.gfx.createShader(skyVertDesc);

		gfx::ShaderDesc skyFragDesc;
		skyFragDesc.stage = gfx::ShaderStage::Fragment;
		skyFragDesc.path = assets.skyFragShader;
		m_skyFragShader = ctx.gfx.createShader(skyFragDesc);

		if (!m_skyFragShader || !m_skyVertShader)
		{
			LOG_ERROR("Sandbox", "Failed to load sky shaders");
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
		skyPipelineDesc.vertexShader = m_skyVertShader.get();
		skyPipelineDesc.fragmentShader = m_skyFragShader.get();
		skyPipelineDesc.rasterizerState.cullMode = gfx::CullMode::None;
		skyPipelineDesc.depthStencilState.depthTestEnable = true;
		skyPipelineDesc.depthStencilState.depthWriteEnable = false;
		skyPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::LessOrEqual;
		skyPipelineDesc.blendState.blendEnable = false;
		skyPipelineDesc.colourFormat = m_hdrColourFormat;
		skyPipelineDesc.depthFormat = m_hdrDepthFormat;
		skyPipelineDesc.sampleCount = kMsaaSampleCount;
		skyPipelineDesc.hasInstanceBinding = false;
		m_skyPipeline = ctx.gfx.createPipeline(skyPipelineDesc);

		if (!m_skyPipeline)
		{
			LOG_ERROR("Sandbox", "Failed to create sky pipeline");
			return false;
		}

		gfx::PipelineDesc meshPipelineDesc{};
		meshPipelineDesc.vertexShader = m_meshVertShader.get();
		meshPipelineDesc.fragmentShader = m_meshFragShader.get();
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
		m_pipeline = ctx.gfx.createPipeline(meshPipelineDesc);

		gfx::PipelineDesc blendPipelineDesc{ meshPipelineDesc };
		blendPipelineDesc.blendState.blendEnable = true;
		blendPipelineDesc.depthStencilState.depthTestEnable = true;
		blendPipelineDesc.depthStencilState.depthWriteEnable = false;
		blendPipelineDesc.colourFormat = m_hdrColourFormat;
		m_blendPipeline = ctx.gfx.createPipeline(blendPipelineDesc);

		gfx::PipelineDesc tonemapPipelineDesc;
		tonemapPipelineDesc.vertexShader = m_tonemapVertShader.get();
		tonemapPipelineDesc.fragmentShader = m_tonemapFragShader.get();
		tonemapPipelineDesc.colourFormat = ctx.gfx.backBuffer().format();
		tonemapPipelineDesc.depthFormat = gfx::TextureFormat::Unknown;
		m_tonemapPipeline = ctx.gfx.createPipeline(tonemapPipelineDesc);

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

		gfx::PipelineDesc shadowPipelineDesc{};
		shadowPipelineDesc.vertexShader = m_shadowVertShader.get();
		shadowPipelineDesc.fragmentShader = m_shadowFragShader.get();
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
		m_shadowPipeline = ctx.gfx.createPipeline(shadowPipelineDesc);

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

		for (auto& buf : m_cascadeUBOs) buf.reset();
		for (auto& buf : m_lightUBOs) buf.reset();
		for (auto& buf : m_instanceBuffers) buf.reset();

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
