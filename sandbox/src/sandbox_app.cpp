#include <sandbox/sandbox_app.h>

#include <core/log/log.h>

#include <algorithm>
#include <cstddef>
#include <cstring>

#include <gfx/model_renderer.h>
#include <gfx/render_extraction.h>
#include <gfx/lighting.h>
#include <gfx/model.h>
#include <gfx/model_loader.h>
#include <gfx/config.h>

namespace imp::app
{
	bool SandboxApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 4.f });
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

		gfx::ShaderDesc skyVertDesc;
		skyVertDesc.stage = gfx::ShaderStage::Vertex;
		skyVertDesc.path = "assets/shaders/sky.vert.spv";
		m_skyVertShader = ctx.gfx.createShader(skyVertDesc);

		gfx::ShaderDesc skyFragDesc;
		skyFragDesc.stage = gfx::ShaderStage::Fragment;
		skyFragDesc.path = "assets/shaders/sky.frag.spv";
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

		ensureHdrTargetSize(ctx);

		gfx::PipelineDesc skyPipelineDesc{};
		skyPipelineDesc.vertexShader = m_skyVertShader.get();
		skyPipelineDesc.fragmentShader = m_skyFragShader.get();
		skyPipelineDesc.rasterizerState.cullMode = gfx::CullMode::None;
		skyPipelineDesc.depthStencilState.depthTestEnable = true;
		skyPipelineDesc.depthStencilState.depthWriteEnable = false;
		skyPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::LessOrEqual;
		skyPipelineDesc.blendState.blendEnable = false;
		skyPipelineDesc.colourFormat = m_hdrTarget->format();
		skyPipelineDesc.depthFormat = m_hdrDepthTarget->format();
		skyPipelineDesc.sampleCount = kMsaaSampleCount;
		skyPipelineDesc.pushConstantSize = sizeof(gfx::SkyPushConstants);
		skyPipelineDesc.hasUniformBuffer = false;
		skyPipelineDesc.hasInstanceBinding = false;
		skyPipelineDesc.textureCount = 0;
		skyPipelineDesc.hasMaterialUniformBuffer = false;

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
		meshPipelineDesc.colourFormat = m_hdrTarget->format();
		meshPipelineDesc.depthFormat = m_hdrDepthTarget->format();
		meshPipelineDesc.sampleCount = kMsaaSampleCount;
		meshPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
		meshPipelineDesc.hasUniformBuffer = true;
		meshPipelineDesc.hasInstanceBinding = true;
		meshPipelineDesc.textureCount = 5;
		meshPipelineDesc.hasMaterialUniformBuffer = true;
		meshPipelineDesc.hasCascadeUniformBuffer = true;
		
		m_pipeline = ctx.gfx.createPipeline(meshPipelineDesc);

		gfx::PipelineDesc blendPipelineDesc{ meshPipelineDesc };
		blendPipelineDesc.blendState.blendEnable = true;
		blendPipelineDesc.depthStencilState.depthTestEnable = true;
		blendPipelineDesc.depthStencilState.depthWriteEnable = false;
		blendPipelineDesc.colourFormat = m_hdrTarget->format();
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
		samplerDesc.enableAnisotropy = true;
		m_sampler = ctx.gfx.createSampler(samplerDesc);

		gfx::TextureDesc cascadeDesc;
		cascadeDesc.width = m_cascadeConfig.shadowMapResolution;
		cascadeDesc.height = m_cascadeConfig.shadowMapResolution;
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
		shadowPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
		shadowPipelineDesc.hasUniformBuffer = false;
		shadowPipelineDesc.hasInstanceBinding = true;
		shadowPipelineDesc.textureCount = 0;
		shadowPipelineDesc.hasMaterialUniformBuffer = false;

		m_shadowPipeline = ctx.gfx.createPipeline(shadowPipelineDesc);

		m_environmentHandle = m_modelRegistry.load(ctx.gfx, "assets/models/khr-sponza.glb", ctx.jobs, &ctx.vfs);
		if (!m_environmentHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment model");

		m_environmentTestHandle = m_modelRegistry.load(ctx.gfx, "assets/models/environment_test.glb", ctx.jobs, &ctx.vfs);
		if (!m_environmentTestHandle.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment test model");

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

		if (!m_pipeline || !m_blendPipeline || !m_hdrTarget || !m_tonemapPipeline || !m_sampler 
			|| !m_environmentHandle.isValid() || !m_shadowPipeline || !m_shadowSampler || !m_hdrDepthTarget)
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
		m_sunEntity = sunEntity;
		m_instances.push_back(sunEntity);

		m_localLight = ctx.ecs.createEntity();
		ecs::Transform pointTransform;
		pointTransform.position = math::Vec3f{ 0.f, 5.f, 0.f };
		ctx.ecs.transforms.create(m_localLight, pointTransform);
		ctx.ecs.colliders.createAABB(m_localLight, math::Vec3f{ -1.f, -1.f, -1.f }, math::Vec3f{ 1.f, 1.f, 1.f });
		ctx.ecs.lights.create(m_localLight, ecs::LightType::Point, math::Vec3f{ 1.f, 0.6f, 0.3f }, 1.5f);
		m_instances.push_back(m_localLight);

		{
			ecs::Transform t;
			spawnInstance(ctx, t, m_environmentTestHandle);
		}

		const ecs::EntityId entity = ctx.ecs.createEntity();
		ecs::Transform t;
		t.position = math::Vec3f{ 5.f, 0.f, 5.f };
		ctx.ecs.transforms.create(entity, t);
		ctx.ecs.renderables.create(entity, m_environmentHandle);
		ctx.ecs.colliders.createAABB(entity, math::Vec3f{ -1.f, -1.f, -1.f }, math::Vec3f{ 1.f, 1.f, 1.f });
		m_instances.push_back(entity);

		return true;
	}

	void SandboxApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);

		ctx.ecs.transforms.updateWorldMatricesParallel(ctx.jobs);
		ctx.ecs.transforms.setLocalTransform(m_localLight, m_localLightTransform);

		updateSunViewProj();
		extractRenderables(ctx.ecs, m_modelRegistry, m_camera.position(), m_extraction);

		ensureInstanceBufferCapacity(ctx, static_cast<u32>( m_extraction.instanceData.size() ));
		if (!m_instanceBuffers.empty() && !m_extraction.instanceData.empty())
		{
			u32 currentFrame = ctx.gfx.currentFrameIndex();
			std::memcpy(m_instanceBuffers[currentFrame]->mappedData(), m_extraction.instanceData.data(),
				m_extraction.instanceData.size() * sizeof(math::Mat4f));
		}
	}

	void SandboxApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
	{
		ensureHdrTargetSize(ctx);

		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		m_cascades = gfx::computeCascades(m_camera, aspect, m_sunDirection, m_cascadeConfig);

		gfx::CascadeUBO cascadeData{};
		for (u32 i = 0; i < gfx::kCascadeCount; ++i)
		{
			cascadeData.viewProj[i] = m_cascades[i].viewProj;
			cascadeData.splitDepths[i] = m_cascades[i].splitDepth;
		}
		cascadeData.blendParams = math::Vec4f{ m_camera.nearPlane, m_cascadeConfig.blendFraction, 0.f, 0.f };
		u32 currentFrame = ctx.gfx.currentFrameIndex();
		m_cascadeUBOs[currentFrame]->update(&cascadeData, sizeof(cascadeData));

		m_extraction.lightData.sunViewProj = m_sunViewProj;
		m_extraction.lightData.shadowMapSize = static_cast<float>( m_cascadeConfig.shadowMapResolution );
		m_lightUBOs[currentFrame]->update(&m_extraction.lightData, sizeof(gfx::LightUBO));

		for (u32 i = 0; i < gfx::kCascadeCount; ++i)
		{
			gfx::RenderPassDesc shadowPassDesc{};
			shadowPassDesc.colourTarget = nullptr;
			shadowPassDesc.depthTarget = m_shadowCascadeTargets[i].get();
			shadowPassDesc.clearDepthValue = 1.f;
			cmd.beginRenderPass(shadowPassDesc);

			gfx::CullVolume cullVolume{ m_cascades[i].lightView, m_cascades[i].boxMin, m_cascades[i].boxMax };

			gfx::ModelRenderContext shadowRenderCtx{};
			shadowRenderCtx.cmd = &cmd;
			shadowRenderCtx.modelRegistry = &m_modelRegistry;
			shadowRenderCtx.sampler = m_sampler.get();
			shadowRenderCtx.lightBuffer = nullptr;
			shadowRenderCtx.instanceBuffer = m_instanceBuffers[currentFrame].get();
			shadowRenderCtx.viewProj = m_cascades[i].viewProj;
			shadowRenderCtx.cullVolume = &cullVolume;

			cmd.bindPipeline(*m_shadowPipeline);
			drawModelBatches(shadowRenderCtx, m_extraction);
			cmd.endRenderPass();
		}

		gfx::RenderPassDesc hdrPassDesc{};
		hdrPassDesc.colourTarget = m_hdrTarget.get();
		hdrPassDesc.resolveTarget = m_hdrResolveTarget.get();
		hdrPassDesc.depthTarget = m_hdrDepthTarget.get();
		hdrPassDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };
		hdrPassDesc.clearDepthValue = 1.f;
		cmd.beginRenderPass(hdrPassDesc);

		gfx::ModelRenderContext renderCtx{};
		renderCtx.cmd = &cmd;
		renderCtx.modelRegistry = &m_modelRegistry;
		renderCtx.sampler = m_sampler.get();
		renderCtx.lightBuffer = m_lightUBOs[currentFrame].get();
		renderCtx.instanceBuffer = m_instanceBuffers[currentFrame].get();
		renderCtx.viewProj = m_camera.projection(aspect) * m_camera.view();
		renderCtx.shadowArrayTexture = m_shadowArrayTexture;
		renderCtx.cascadeBuffer = m_cascadeUBOs[currentFrame].get();
		renderCtx.shadowSampler = m_shadowSampler.get();

		cmd.bindPipeline(*m_pipeline);
		drawModelBatches(renderCtx, m_extraction);

		gfx::SkyPushConstants skyPC{};
		skyPC.invViewProj = math::inverse(renderCtx.viewProj);
		skyPC.cameraPositionWS = math::Vec4f{ m_camera.position(), 1.f };
		skyPC.sunDirAndIntensity = math::Vec4f{ -m_sunDirection, 10.f };

		cmd.bindPipeline(*m_skyPipeline);
		cmd.pushConstants(&skyPC, sizeof(skyPC));
		cmd.draw(3, 1);

		if (!m_extraction.blendInstances.empty())
		{
			cmd.bindPipeline(*m_blendPipeline);
			drawBlendInstances(renderCtx, m_extraction);
		}

		ctx.layers.renderAll(cmd);

		cmd.endRenderPass();

		gfx::RenderPassDesc tonemapPassDesc{};
		tonemapPassDesc.colourTarget = &ctx.gfx.backBuffer();
		tonemapPassDesc.depthTarget = nullptr;
		tonemapPassDesc.clearColour = false;
		cmd.beginRenderPass(tonemapPassDesc);

		cmd.bindPipeline(*m_tonemapPipeline);
		cmd.bindTexture(*m_hdrResolveTarget->asTexture(), *m_sampler, 1);
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

		m_sampler.reset();
		m_pipeline.reset();
		m_tonemapPipeline.reset();
		m_blendPipeline.reset();
		m_hdrTarget.reset();
		m_hdrDepthTarget.reset();
		m_hdrResolveTarget.reset();
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

		for (auto& buf : m_cascadeUBOs)
			buf.reset();

		for (auto& buf : m_lightUBOs)
			buf.reset();

		for (auto& buf : m_instanceBuffers)
			buf.reset();
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
		if (!m_instanceBuffers.empty() && instanceCount <= m_instanceCapacity)
			return;

		u32 newCapacity = std::max<u32>(instanceCount, m_instanceCapacity * 2);
		newCapacity = std::max<u32>(newCapacity, 16u);

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

	ecs::EntityId SandboxApp::spawnInstance(AppContext& ctx, const ecs::Transform& t, const gfx::ModelHandle& model)
	{
		const ecs::EntityId entity = ctx.ecs.createEntity();
		ctx.ecs.transforms.create(entity, t);
		ctx.ecs.renderables.create(entity, model);
		ctx.ecs.colliders.createAABB(entity, math::Vec3f{ -0.5f, -0.5f, -0.5f }, math::Vec3f{ 0.5f, 0.5f, 0.5f });
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

		ctx.gfx.waitIdle();

		gfx::TextureDesc hdrDesc;
		hdrDesc.width = w;
		hdrDesc.height = h;
		hdrDesc.format = gfx::TextureFormat::RGBA16Float;
		hdrDesc.usage = gfx::TextureUsage::RenderTarget;
		hdrDesc.sampleCount = kMsaaSampleCount;
		auto newTarget = ctx.gfx.createRenderTarget(hdrDesc);

		gfx::TextureDesc resolveDesc;
		resolveDesc.width = w;
		resolveDesc.height = h;
		resolveDesc.format = gfx::TextureFormat::RGBA16Float;
		resolveDesc.usage = gfx::TextureUsage::Sampled | gfx::TextureUsage::RenderTarget;
		resolveDesc.sampleCount = gfx::SampleCount::One;
		auto newResolveTarget = ctx.gfx.createRenderTarget(resolveDesc);

		gfx::TextureDesc hdrDepthDesc;
		hdrDepthDesc.width = w;
		hdrDepthDesc.height = h;
		hdrDepthDesc.format = ctx.gfx.depthBuffer() ? ctx.gfx.depthBuffer()->format() : gfx::TextureFormat::Depth32Float;
		hdrDepthDesc.usage = gfx::TextureUsage::DepthStencil; // never sampled
		hdrDepthDesc.sampleCount = kMsaaSampleCount;
		auto newDepthTarget = ctx.gfx.createRenderTarget(hdrDepthDesc);

		if (newTarget && newResolveTarget && newDepthTarget)
		{
			m_hdrTarget = std::move(newTarget);
			m_hdrResolveTarget = std::move(newResolveTarget);
			m_hdrDepthTarget = std::move(newDepthTarget);
		}
		else
			LOG_ERROR("Sandbox", "Failed to recreate HDR/resolve/depth targets at {}x{}", w, h);
	}
}
