#include <sandbox/sandbox_app.h>

#include <core/log/log.h>

#include <cstddef>
#include <cstring>

#include <gfx/lighting.h>
#include <gfx/model.h>
#include <gfx/model_loader.h>

namespace imp::app
{
	bool SandboxApp::onInit(AppContext& ctx)
	{
		m_camera.setPosition({ 0.f, 1.f, 0.f });
		m_camera.setYawPitch(math::toRadians(90.f), 0.f);

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
			LOG_ERROR("Sandbox", "Failed to load mesh shaders");
			return false;
		}

		gfx::VertexAttribute attrs[3] = {
			{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
			{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
			{ 2, static_cast<u32>( offsetof(gfx::ModelVertex, uv) ), 2, true },
		};

		gfx::PipelineDesc meshPipelineDesc;
		meshPipelineDesc.vertexShader = m_meshVertShader.get();
		meshPipelineDesc.fragmentShader = m_meshFragShader.get();
		meshPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
		meshPipelineDesc.vertexLayout.attributeCount = 3;
		meshPipelineDesc.vertexLayout.attributes = attrs;
		meshPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
		meshPipelineDesc.depthStencilState.depthTestEnable = true;
		meshPipelineDesc.depthStencilState.depthWriteEnable = true;
		meshPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
		meshPipelineDesc.colourFormat = ctx.gfx.backBuffer().format();
		meshPipelineDesc.depthFormat = ctx.gfx.depthBuffer() ? ctx.gfx.depthBuffer()->format() : gfx::TextureFormat::Unknown;
		meshPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
		meshPipelineDesc.hasUniformBuffer = true;
		meshPipelineDesc.hasTexture = true;
		
		m_pipeline = ctx.gfx.createPipeline(meshPipelineDesc);

		gfx::SamplerDesc samplerDesc;
		samplerDesc.minFilter = gfx::FilterMode::Linear;
		samplerDesc.magFilter = gfx::FilterMode::Linear;
		samplerDesc.addressModeU = gfx::AddressMode::Repeat;
		samplerDesc.addressModeV = gfx::AddressMode::Repeat;
		m_sampler = ctx.gfx.createSampler(samplerDesc);

		m_model = gfx::loadModel(ctx.gfx, "assets/models/lonely_watcher_by_artjoms_horosilovs.glb", &ctx.vfs);
		if (!m_model.isValid())
			LOG_ERROR("Sandbox", "Failed to load environment model");

		m_statue = gfx::loadModel(ctx.gfx, "assets/models/statue.glb", &ctx.vfs);
		if (!m_statue.isValid())
			LOG_ERROR("Sandbox", "Failed to load statue model");

		gfx::BufferDesc lightUboDesc;
		lightUboDesc.size = sizeof(gfx::BlinnPhongLightUBO);
		lightUboDesc.usage = gfx::BufferUsage::Uniform;
		lightUboDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
		m_lightBuffer = ctx.gfx.createBuffer(lightUboDesc);

		if (!m_pipeline || !m_sampler || !m_lightBuffer || !m_model.isValid())
		{
			LOG_FATAL("Sandbox", "Failed to create pipeline/sampler/light buffer, or model failed to load");
			return false;
		}

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

		/*ecs::Transform t;
		t.scale = math::Vec3f{ 0.01f, 0.01f, 0.01f };
		spawnInstance(ctx, t);*/

		return true;
	}

	void SandboxApp::onUpdate(AppContext& ctx, float deltaSeconds)
	{
		m_camera.update(ctx.input, deltaSeconds);

		gfx::BlinnPhongLightUBO lightData;
		lightData.cameraPositionWS = { m_camera.position().x, m_camera.position().y, m_camera.position().z, 0.f };
		std::memcpy(m_lightBuffer->mappedData(), &lightData, sizeof(lightData));

		ctx.ecs.transforms.updateWorldMatricesParallel(ctx.jobs);
	}

	void SandboxApp::onRender(AppContext& ctx, gfx::ICommandList& cmd)
	{
		gfx::RenderPassDesc passDesc;
		passDesc.colourTarget = &ctx.gfx.backBuffer();
		passDesc.depthTarget = ctx.gfx.depthBuffer();
		passDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };
		passDesc.clearDepthValue = 1.f;

		cmd.beginRenderPass(passDesc);
		cmd.bindPipeline(*m_pipeline);

		const u32 w = ctx.gfx.backBuffer().width();
		const u32 h = ctx.gfx.backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		math::Mat4f viewProj = m_camera.projection(aspect) * m_camera.view();
		//math::Mat4f worldRoot{};

		for (ecs::EntityId instance : m_instances)
		{
			const math::Mat4f& instanceWorld = ctx.ecs.transforms.worldMatrix(instance);
			for (u32 root : m_statue.rootNodes)
				drawNode(cmd, m_statue, viewProj, root, instanceWorld);
		}

		/*for (u32 root : m_model.rootNodes)
			drawNode(cmd, m_model, viewProj, root, worldRoot);*/

		cmd.endRenderPass();
	}

	void SandboxApp::onShutdown(AppContext& ctx)
	{
		for (ecs::EntityId instance : m_instances)
			ctx.ecs.destroyEntity(instance);

		m_instances.clear();

		m_model = gfx::Model{};
		m_statue = gfx::Model{};

		m_lightBuffer.reset();
		m_sampler.reset();
		m_pipeline.reset();
		m_meshFragShader.reset();
		m_meshVertShader.reset();
	}

	void SandboxApp::drawNode(gfx::ICommandList& cmd, gfx::Model& model, const math::Mat4f& viewProj, u32 nodeIdx, const math::Mat4f& parentWorld)
	{
		const gfx::ModelNode& node = model.nodes[nodeIdx];
		math::Mat4f world = parentWorld * node.localTransform;

		if (node.meshIndex >= 0)
		{
			for (auto& prim : model.meshes[node.meshIndex].primitives)
			{
				gfx::MeshPushConstants pc;
				pc.model = world;
				pc.mvp = viewProj * world;
				cmd.pushConstants(&pc, sizeof(pc), 0);

				cmd.bindUniformBuffer(*m_lightBuffer, 0);

				if (prim.materialIndex >= 0)
				{
					i32 texIdx = model.materials[prim.materialIndex].baseColourTextureIndex;
					if (texIdx >= 0)
						cmd.bindTexture(*model.textures[texIdx].texture, *m_sampler, 1);
				}

				cmd.bindVertexBuffer(*prim.vertexBuffer);
				cmd.bindIndexBuffer(*prim.indexBuffer);
				cmd.drawIndexed(prim.indexCount, 1);
			}
		}

		for (u32 child : node.children)
			drawNode(cmd, model, viewProj, child, world);
	}

	ecs::EntityId SandboxApp::spawnInstance(AppContext& ctx, const ecs::Transform& t)
	{
		const ecs::EntityId entity = ctx.ecs.createEntity();
		ctx.ecs.transforms.create(entity, t);
		m_instances.push_back(entity);
		return entity;
	}
}
