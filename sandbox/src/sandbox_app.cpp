#include <sandbox/sandbox_app.h>

#include <core/log/log.h>

#include <algorithm>
#include <cstddef>
#include <cstring>

#include <app/model_renderer.h>
#include <app/render_extraction.h>
#include <gfx/lighting.h>
#include <gfx/model.h>
#include <gfx/model_loader.h>

namespace imp::app
{
	bool SandboxApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 25.f, 1.f, 4.f });
		m_camera.setYawPitch(math::toRadians(-90.f), 0.f);

		gfx::ShaderDesc meshVertDesc;
		meshVertDesc.stage = gfx::ShaderStage::Vertex;
		meshVertDesc.path = "assets/shaders/mesh.vert.spv";
		m_meshVertShader = ctx.gfx.createShader(meshVertDesc);

		gfx::ShaderDesc meshFragDesc;
		meshFragDesc.stage = gfx::ShaderStage::Fragment;
		meshFragDesc.path = "assets/shaders/mesh.frag.spv";
		m_meshFragShader = ctx.gfx.createShader(meshFragDesc);

		if (!m_meshFragShader || !m_meshVertShader)
		{
			LOG_ERROR("Sandbox", "Failed to load mesh shaders.");
			return false;
		}

		gfx::ShaderDesc tonemapVertDesc;
		tonemapVertDesc.stage = gfx::ShaderStage::Vertex;
		tonemapVertDesc.path = "assets/shaders/tonemap.vert.spv";
		m_tonemapVertShader = ctx.gfx.createShader(tonemapVertDesc);

		gfx::ShaderDesc tonemapFragDesc;
		tonemapFragDesc.stage = gfx::ShaderStage::Fragment;
		tonemapFragDesc.path = "assets/shaders/tonemap.frag.spv";
		m_tonemapFragShader = ctx.gfx.createShader(tonemapFragDesc);

		if (!m_tonemapVertShader || !m_tonemapFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load tonemap shaders.");
			return false;
		}

		gfx::ShaderDesc shadowVertDesc;
		shadowVertDesc.stage = gfx::ShaderStage::Vertex;
		shadowVertDesc.path = "assets/shaders/shadow.vert.spv";
		m_shadowVertShader = ctx.gfx.createShader(shadowVertDesc);

		gfx::ShaderDesc shadowFragDesc;
		shadowFragDesc.stage = gfx::ShaderStage::Fragment;
		shadowFragDesc.path = "assets/shaders/shadow.frag.spv";
		m_shadowFragShader = ctx.gfx.createShader(shadowFragDesc);

		if (!m_shadowVertShader || !m_shadowFragShader)
		{
			LOG_ERROR("Sandbox", "Failed to load shadow shaders.");
			return false;
		}

		gfx::VertexAttribute meshAttrs[3] = {
			{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
			{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
			{ 2, static_cast<u32>( offsetof(gfx::ModelVertex, uv) ), 2, true },
		};

		gfx::VertexAttribute instanceAttrs[4] = {
			{ 3, static_cast<u32>( sizeof(math::Vec4f) * 0 ), 4, true },
			{ 4, static_cast<u32>( sizeof(math::Vec4f) * 1 ), 4, true },
			{ 5, static_cast<u32>( sizeof(math::Vec4f) * 2 ), 4, true },
			{ 6, static_cast<u32>( sizeof(math::Vec4f) * 3 ), 4, true },
		};

		ensureHdrTargetSize(ctx);

		gfx::PipelineDesc meshPipelineDesc{};
		meshPipelineDesc.vertexShader = m_meshVertShader.get();
		meshPipelineDesc.fragmentShader = m_meshFragShader.get();
		meshPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
		meshPipelineDesc.vertexLayout.attributeCount = 3;
		meshPipelineDesc.vertexLayout.attributes = meshAttrs;
		meshPipelineDesc.instanceLayout.stride = sizeof(math::Mat4f);
		meshPipelineDesc.instanceLayout.attributeCount = 4;
		meshPipelineDesc.instanceLayout.attributes = instanceAttrs;
		meshPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
		meshPipelineDesc.depthStencilState.depthTestEnable = true;
		meshPipelineDesc.depthStencilState.depthWriteEnable = true;
		meshPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		meshPipelineDesc.blendState.blendEnable = false;
		meshPipelineDesc.colourFormat = m_hdrTarget->format();
		meshPipelineDesc.depthFormat = ctx.gfx.depthBuffer() ? ctx.gfx.depthBuffer()->format() : gfx::TextureFormat::Unknown;
		meshPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
		meshPipelineDesc.hasUniformBuffer = true;
		meshPipelineDesc.hasInstanceBinding = true;
		meshPipelineDesc.textureCount = 5;
		meshPipelineDesc.hasMaterialUniformBuffer = true;
		
		m_pipeline = ctx.gfx.createPipeline(meshPipelineDesc);

		gfx::PipelineDesc blendPipelineDesc{ meshPipelineDesc };
		blendPipelineDesc.blendState.blendEnable = true;
		blendPipelineDesc.depthStencilState.depthTestEnable = true;
		blendPipelineDesc.depthStencilState.depthWriteEnable = false;
		blendPipelineDesc.colourFormat = gfx::TextureFormat::RGBA16Float;
		m_blendPipeline = ctx.gfx.createPipeline(blendPipelineDesc);

		gfx::PipelineDesc tonemapPipelineDesc;
		tonemapPipelineDesc.vertexShader = m_tonemapVertShader.get();
		tonemapPipelineDesc.fragmentShader = m_tonemapFragShader.get();
		tonemapPipelineDesc.colourFormat = ctx.gfx.backBuffer().format();
		tonemapPipelineDesc.depthFormat = gfx::TextureFormat::Unknown;
		tonemapPipelineDesc.textureCount = 1;
		m_tonemapPipeline = ctx.gfx.createPipeline(tonemapPipelineDesc);

		gfx::SamplerDesc samplerDesc{};
		samplerDesc.minFilter = gfx::FilterMode::Linear;
		samplerDesc.magFilter = gfx::FilterMode::Linear;
		samplerDesc.addressModeU = gfx::AddressMode::Repeat;
		samplerDesc.addressModeV = gfx::AddressMode::Repeat;
		m_sampler = ctx.gfx.createSampler(samplerDesc);

		gfx::TextureDesc shadowDesc;
		shadowDesc.width = kShadowMapSize;
		shadowDesc.height = kShadowMapSize;
		shadowDesc.format = ctx.gfx.depthBuffer() ? ctx.gfx.depthBuffer()->format() : gfx::TextureFormat::Unknown;
		shadowDesc.usage = gfx::TextureUsage::Sampled | gfx::TextureUsage::DepthStencil;
		m_shadowTarget = ctx.gfx.createRenderTarget(shadowDesc);

		gfx::SamplerDesc shadowSamplerDesc{};
		shadowSamplerDesc.minFilter = gfx::FilterMode::Linear;
		shadowSamplerDesc.magFilter = gfx::FilterMode::Linear;
		shadowSamplerDesc.addressModeU = gfx::AddressMode::ClampToEdge;
		shadowSamplerDesc.addressModeV = gfx::AddressMode::ClampToEdge;
		m_shadowSampler = ctx.gfx.createSampler(shadowSamplerDesc);

		gfx::PipelineDesc shadowPipelineDesc{};
		shadowPipelineDesc.vertexShader = m_shadowVertShader.get();
		shadowPipelineDesc.fragmentShader = m_shadowFragShader.get();
		shadowPipelineDesc.vertexLayout = meshPipelineDesc.vertexLayout;
		shadowPipelineDesc.instanceLayout = meshPipelineDesc.instanceLayout;
		shadowPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Front; // reduces acne on closed meshes
		shadowPipelineDesc.depthStencilState.depthTestEnable = true;
		shadowPipelineDesc.depthStencilState.depthWriteEnable = true;
		shadowPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		shadowPipelineDesc.colourFormat = gfx::TextureFormat::Unknown;
		shadowPipelineDesc.depthFormat = shadowDesc.format;
		shadowPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
		shadowPipelineDesc.hasUniformBuffer = false;
		shadowPipelineDesc.hasInstanceBinding = true;
		shadowPipelineDesc.textureCount = 0;
		shadowPipelineDesc.hasMaterialUniformBuffer = false;

		m_shadowPipeline = ctx.gfx.createPipeline(shadowPipelineDesc);

		m_environmentHandle = m_modelRegistry.load(ctx.gfx, "assets/models/sanmiguel.glb", ctx.jobs, &ctx.vfs);
		if (!m_environmentHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment model");

		m_statueHandle = m_modelRegistry.load(ctx.gfx, "assets/models/statue.glb", ctx.jobs, &ctx.vfs);
		if (!m_statueHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load statue model");

		gfx::BufferDesc lightUboDesc{};
		lightUboDesc.size = sizeof(gfx::LightUBO);
		lightUboDesc.usage = gfx::BufferUsage::Uniform;
		lightUboDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_lightBuffer = ctx.gfx.createBuffer(lightUboDesc);

		if (!m_pipeline || !m_blendPipeline || !m_hdrTarget || !m_tonemapPipeline || !m_sampler 
			|| !m_lightBuffer || !m_environmentHandle.isValid() || !m_shadowPipeline || !m_shadowTarget || !m_shadowSampler)
		{
			LOG_FATAL("Sandbox", "Failed to create pipelines/sampler/light buffer, or model failed to load");
			return false;
		}

		const ecs::EntityId sunEntity = ctx.ecs.createEntity();
		ecs::Transform sunTransform;
		sunTransform.rotation = math::Quaternionf::fromAxisAngle(math::Vec3f::unitX(), math::toRadians(50.f))
			* math::Quaternionf::fromAxisAngle(math::Vec3f::unitY(), math::toRadians(30.f));
		m_sunDirection = math::normalise(math::rotate(sunTransform.rotation, math::Vec3f::forward()));
		ctx.ecs.transforms.create(sunEntity, sunTransform);
		ctx.ecs.lights.create(sunEntity, ecs::LightType::Directional, math::Vec3f{ 1.f, 1.f, 1.f }, 10.f);
		m_instances.push_back(sunEntity);

		m_localLight = ctx.ecs.createEntity();
		ecs::Transform pointTransform;
		pointTransform.position = math::Vec3f{ 0.f, 5.f, 0.f };
		ctx.ecs.transforms.create(m_localLight, pointTransform);
		ctx.ecs.lights.create(m_localLight, ecs::LightType::Point, math::Vec3f{ 1.f, 0.6f, 0.3f }, 0.5f);
		m_instances.push_back(m_localLight);

		constexpr int kGridSize = 3;
		constexpr float kSpacing = 30.f;
		for (int x = 0; x < kGridSize; ++x)
		{
			for (int z = 0; z < kGridSize; ++z)
			{
				const math::Vec3f position
				{
					static_cast<float>(x - kGridSize / 2.f) * kSpacing,
					0.f,
					static_cast<float>(z - kGridSize / 2.f) * kSpacing
				};

				ecs::Transform t;
				t.position = position;
				t.scale = math::Vec3f{ 10.f, 10.f, 10.f };
				spawnInstance(ctx, t);
			}
		}

		const ecs::EntityId entity = ctx.ecs.createEntity();
		ecs::Transform t;
		t.position = math::Vec3f{ 5.f, 0.f, 5.f };
		ctx.ecs.transforms.create(entity, t);
		ctx.ecs.renderables.create(entity, m_environmentHandle);
		m_instances.push_back(entity);

		ensureInstanceBufferCapacity(ctx, static_cast<u32>( m_instances.size() ));
		if (!m_instanceBuffer)
		{
			LOG_FATAL("Sandbox", "Failed to create instance buffer");
			return false;
		}

		return true;
	}

	void SandboxApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);

		ctx.ecs.transforms.updateWorldMatricesParallel(ctx.jobs);

		ctx.ecs.transforms.setLocalTransform(m_localLight, m_localLightTransform);

		updateSunViewProj();
		extractRenderables(ctx.ecs, m_modelRegistry, m_camera.position(), m_extraction);
		m_extraction.lightData.sunViewProj = m_sunViewProj;
		m_extraction.lightData.shadowMapSize = static_cast<float>( kShadowMapSize );
		std::memcpy(m_lightBuffer->mappedData(), &m_extraction.lightData, sizeof(gfx::LightUBO));

		ensureInstanceBufferCapacity(ctx, static_cast<u32>( m_extraction.instanceData.size() ));
		if (m_instanceBuffer && !m_extraction.instanceData.empty())
		{
			std::memcpy(m_instanceBuffer->mappedData(), m_extraction.instanceData.data(),
				m_extraction.instanceData.size() * sizeof(math::Mat4f));
		}
	}

	void SandboxApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
	{
		ensureHdrTargetSize(ctx);

		gfx::RenderPassDesc shadowPassDesc{};
		shadowPassDesc.colourTarget = nullptr;
		shadowPassDesc.depthTarget = m_shadowTarget.get();
		shadowPassDesc.clearDepthValue = 1.f;
		cmd.beginRenderPass(shadowPassDesc);

		ModelRenderContext shadowRenderCtx{};
		shadowRenderCtx.cmd = &cmd;
		shadowRenderCtx.modelRegistry = &m_modelRegistry;
		shadowRenderCtx.sampler = m_sampler.get();
		shadowRenderCtx.lightBuffer = nullptr;
		shadowRenderCtx.instanceBuffer = m_instanceBuffer.get();
		shadowRenderCtx.viewProj = m_sunViewProj;

		cmd.bindPipeline(*m_shadowPipeline);
		drawModelBatches(shadowRenderCtx, m_extraction);

		cmd.endRenderPass();

		gfx::RenderPassDesc hdrPassDesc{};
		hdrPassDesc.colourTarget = m_hdrTarget.get();
		hdrPassDesc.depthTarget = ctx.gfx.depthBuffer();
		hdrPassDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };
		hdrPassDesc.clearDepthValue = 1.f;
		cmd.beginRenderPass(hdrPassDesc);

		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		ModelRenderContext renderCtx{};
		renderCtx.cmd = &cmd;
		renderCtx.modelRegistry = &m_modelRegistry;
		renderCtx.sampler = m_sampler.get();
		renderCtx.lightBuffer = m_lightBuffer.get();
		renderCtx.instanceBuffer = m_instanceBuffer.get();
		renderCtx.viewProj = m_camera.projection(aspect) * m_camera.view();
		renderCtx.shadowMap = m_shadowTarget->asTexture();
		renderCtx.shadowSampler = m_shadowSampler.get();

		cmd.bindPipeline(*m_pipeline);
		drawModelBatches(renderCtx, m_extraction);

		if (!m_extraction.blendInstances.empty())
		{
			cmd.bindPipeline(*m_blendPipeline);
			drawBlendInstances(renderCtx, m_extraction);
		}

		cmd.endRenderPass();

		gfx::RenderPassDesc tonemapPassDesc{};
		tonemapPassDesc.colourTarget = &ctx.gfx.backBuffer();
		tonemapPassDesc.depthTarget = nullptr;
		tonemapPassDesc.clearColour = false;
		cmd.beginRenderPass(tonemapPassDesc);

		cmd.bindPipeline(*m_tonemapPipeline);
		cmd.bindTexture(*m_hdrTarget->asTexture(), *m_sampler, 1);
		cmd.draw(3, 1);

		cmd.endRenderPass();
	}

	void SandboxApp::onShutdown(AppContext& ctx)
	{
		for (ecs::EntityId instance : m_instances)
			ctx.ecs.destroyEntity(instance);

		m_instances.clear();
		m_extraction.clear();

		m_modelRegistry.shutdown();
		m_modelRegistry.clear();

		m_instanceBuffer.reset();
		m_lightBuffer.reset();
		m_sampler.reset();
		m_pipeline.reset();
		m_tonemapPipeline.reset();
		m_blendPipeline.reset();
		m_hdrTarget.reset();
		m_tonemapFragShader.reset();
		m_tonemapVertShader.reset();
		m_meshFragShader.reset();
		m_meshVertShader.reset();
		m_shadowPipeline.reset();
		m_shadowTarget.reset();
		m_shadowSampler.reset();
		m_shadowFragShader.reset();
		m_shadowVertShader.reset();
	}

	void SandboxApp::updateSunViewProj()
	{
		using namespace imp::math;

		Vec3f sunDir = normalise(m_sunDirection);
		Vec3f up = std::abs(dot(sunDir, Vec3f::up())) > 0.99f ? Vec3f::unitX() : Vec3f::up();

		constexpr float kSceneRadius = 80.f;
		const Vec3f sceneCentre = Vec3f::zero();
		const Vec3f eye = sceneCentre - sunDir * kSceneRadius;

		Mat4f lightView = makeLookAtLH(eye, sceneCentre, up);
		Mat4f lightProj = makeOrthographicOffcentreLH(-kSceneRadius, kSceneRadius, -kSceneRadius, kSceneRadius, 0.1f, kSceneRadius * 2.f);

		m_sunViewProj = lightProj * lightView;
	}

	void SandboxApp::ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount)
	{
		if (m_instanceBuffer && instanceCount <= m_instanceCapacity)
			return;

		u32 newCapacity = std::max<u32>(instanceCount, m_instanceCapacity * 2);
		newCapacity = std::max<u32>(newCapacity, 16u);

		gfx::BufferDesc desc;
		desc.size = static_cast<u64>( newCapacity ) * sizeof(math::Mat4f);
		desc.usage = gfx::BufferUsage::Vertex;
		desc.memoryAccess = gfx::MemoryAccess::HostVisible;
		desc.debugName = "SandboxApp instance buffer";

		auto newBuffer = ctx.gfx.createBuffer(desc);
		if (!newBuffer)
		{
			LOG_ERROR("Sandbox", "Failed to create instance buffer for capacity {}", newCapacity);
			return;
		}

		m_instanceBuffer = std::move(newBuffer);
		m_instanceCapacity = newCapacity;
	}

	ecs::EntityId SandboxApp::spawnInstance(AppContext& ctx, const ecs::Transform& t)
	{
		const ecs::EntityId entity = ctx.ecs.createEntity();
		ctx.ecs.transforms.create(entity, t);
		ctx.ecs.renderables.create(entity, m_statueHandle);
		m_instances.push_back(entity);
		return entity;
	}

	void SandboxApp::ensureHdrTargetSize(AppContext& ctx)
	{
		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();

		if (m_hdrTarget && m_hdrTarget->width() == w && m_hdrTarget->height() == h)
			return;

		if (w == 0 || h == 0)
			return;

		gfx::TextureDesc hdrDesc;
		hdrDesc.width = w;
		hdrDesc.height = h;
		hdrDesc.format = gfx::TextureFormat::RGBA16Float;
		hdrDesc.usage = gfx::TextureUsage::Sampled | gfx::TextureUsage::RenderTarget;
		auto newTarget = ctx.gfx.createRenderTarget(hdrDesc);
		if (newTarget)
			m_hdrTarget = std::move(newTarget);
		else
			LOG_ERROR("Sandbox", "Failed to recreate HDR target at {}x{}", w, h);
	}
}
