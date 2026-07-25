#include <core/log/log.h>
#include <core/memory/heap_allocator.h>

#include <fwk/window.h>
#include <fwk/camera.h>

#include <gfx/gfx.h>
#include <gfx/image.h>
#include <gfx/model.h>
#include <gfx/model_loader.h>
#include <gfx/lighting.h>

#include <memory>
#include <functional>
#include <cstring>

#include <core/math/math.h>

#include <core/fs/vfs.h>
#include <core/platform/exe_path.h>

#include <protocol/tool_server.h>
#include <protocol/memory_telemetry.h>

using namespace imp;

int main()
{
	log::Logger::get().initialise();
	protocol::ToolServer::instance().start(47810);

	LOG_INFO("Sandbox", "App starting...");

	memory::HeapAllocator gfxHostAllocator("GfxHost");
	fs::VirtualFileSystem vfsHost;

	const auto shadersPath = ( imp::platform::executableDir() / "assets" ).string();
	if (!vfsHost.mount("assets/", shadersPath, 0, true, true))
	{
		LOG_ERROR("Sandbox", "Failed to mount shaders");
		return 1;
	}

	fwk::Window window;
	fwk::WindowDesc windowDesc{};
#ifndef NDEBUG
	windowDesc.title = "Atlas";
#else
	windowDesc.title = "Velvet";
#endif
	windowDesc.width = 1280;
	windowDesc.height = 720;

	if (!window.create(windowDesc))
	{
		LOG_ERROR("Sandbox", "Failed to create window");
		return 1;
	}

	std::unique_ptr<gfx::IDevice> gfx;
	for (gfx::GraphicsApi api : gfx::availableApis())
	{
		std::unique_ptr<gfx::IDevice> candidate = gfx::createDevice(api);
		if (!candidate)
			continue;

		gfx::DeviceDesc gfxDesc;
		gfxDesc.window = &window;
		gfxDesc.appName = "Sandbox";
		gfxDesc.allocator = &gfxHostAllocator;
		gfxDesc.vfs = &vfsHost;
#ifndef NDEBUG
		gfxDesc.enableValidation = true;
#endif

		if (candidate->initialise(gfxDesc))
		{
			gfx = std::move(candidate);
			break;
		}
		LOG_WARN("Sandbox", "{} did not initialise, trying next available API", gfx::toString(api));
	}

	if (!gfx)
	{
		LOG_FATAL("Sandbox", "No available graphics API could be initialised");
		window.destroy();
		return 1;
	}

	LOG_INFO("Sandbox", "Running with {} device, window ({}, {})", gfx->apiName(), window.width(), window.height());

	fwk::Camera camera;
	camera.setPosition({ 0.f, 1.f, 0.f });
	camera.setYawPitch(math::toRadians(90.f), 0.f);

	gfx::ShaderDesc meshVertDesc;
	meshVertDesc.stage = gfx::ShaderStage::Vertex;
	meshVertDesc.path = "assets/shaders/mesh.vert.spv";
	std::unique_ptr<gfx::IShader> meshVertShader = gfx->createShader(meshVertDesc);

	gfx::ShaderDesc meshFragDesc;
	meshFragDesc.stage = gfx::ShaderStage::Fragment;
	meshFragDesc.path = "assets/shaders/mesh.frag.spv";
	std::unique_ptr<gfx::IShader> meshFragShader = gfx->createShader(meshFragDesc);

	if (!meshFragShader || !meshVertShader)
	{
		LOG_ERROR("Sandbox", "Failed to load mesh shaders");
		gfx->shutdown();
		window.destroy();
		return 1;
	}

	gfx::VertexAttribute attrs[3] = {
		{ 0, static_cast<u32>( offsetof(gfx::ModelVertex, position) ), 3, true },
		{ 1, static_cast<u32>( offsetof(gfx::ModelVertex, normal) ), 3, true },
		{ 2, static_cast<u32>( offsetof(gfx::ModelVertex, uv) ), 2, true },
	};

	gfx::PipelineDesc meshPipelineDesc;
	meshPipelineDesc.vertexShader = meshVertShader.get();
	meshPipelineDesc.fragmentShader = meshFragShader.get();
	meshPipelineDesc.vertexLayout.stride = sizeof(gfx::ModelVertex);
	meshPipelineDesc.vertexLayout.attributeCount = 3;
	meshPipelineDesc.vertexLayout.attributes = attrs;
	meshPipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
	meshPipelineDesc.depthStencilState.depthTestEnable = true;
	meshPipelineDesc.depthStencilState.depthWriteEnable = true;
	meshPipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
	meshPipelineDesc.colourFormat = gfx->backBuffer().format();
	meshPipelineDesc.depthFormat = gfx->depthBuffer() ? gfx->depthBuffer()->format() : gfx::TextureFormat::Unknown;
	meshPipelineDesc.pushConstantSize = sizeof(gfx::MeshPushConstants);
	meshPipelineDesc.hasUniformBuffer = true;
	meshPipelineDesc.hasTexture = true;

	std::unique_ptr<gfx::IPipeline> pipeline = gfx->createPipeline(meshPipelineDesc);

	gfx::SamplerDesc samplerDesc;
	samplerDesc.minFilter = gfx::FilterMode::Linear;
	samplerDesc.magFilter = gfx::FilterMode::Linear;
	samplerDesc.addressModeU = gfx::AddressMode::Repeat;
	samplerDesc.addressModeV = gfx::AddressMode::Repeat;
	std::unique_ptr<gfx::ISampler> sampler = gfx->createSampler(samplerDesc);

	gfx::Model model = gfx::loadModel(*gfx, "assets/models/sponza.glb", &vfsHost);
	if (!model.isValid())
		LOG_ERROR("Sandbox", "Failed to load model");

	gfx::BufferDesc lightUboDesc;
	lightUboDesc.size = sizeof(gfx::BlinnPhongLightUBO);
	lightUboDesc.usage = gfx::BufferUsage::Uniform;
	lightUboDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
	std::unique_ptr<gfx::IBuffer> lightBuffer = gfx->createBuffer(lightUboDesc);

	if (!pipeline || !sampler || !lightBuffer || !model.isValid())
	{
		LOG_FATAL("Sandbox", "Failed to create pipeline/sampler/light buffer, or model failed to load");
		gfx->shutdown();
		window.destroy();
		return 1;
	}

	// Tool server stuff
	auto lastTelemetryPublish = std::chrono::steady_clock::now();
	constexpr auto kTelemetryInterval = std::chrono::milliseconds(200);

	auto lastFrameTime = std::chrono::steady_clock::now();

	while (!window.shouldClose())
	{
		window.input().newFrame();
		window.pollEvents();

		if (window.isMinimised())
			continue;

		const auto now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
		lastFrameTime = now;

		if (now - lastTelemetryPublish >= kTelemetryInterval &&
			imp::protocol::ToolServer::instance().hasSubscribers(imp::protocol::MessageType::MemoryTelemetry))
		{
			const auto snap = gfxHostAllocator.statsSnapshot();

			imp::protocol::AllocatorStatsPayload payload;
			payload.name = std::string(gfxHostAllocator.name());
			payload.totalAllocated = snap.totalAllocated;
			payload.totalFreed = snap.totalFreed;
			payload.currentUsed = snap.currentUsed;
			payload.peakUsed = snap.peakUsed;
			payload.allocationCount = snap.allocationCount;
			payload.freeCount = snap.freeCount;
			payload.tagBytes.assign(std::begin(snap.tagBytes), std::end(snap.tagBytes));

			const auto bytes = imp::protocol::serialiseMemoryTelemetry({ payload });
			imp::protocol::ToolServer::instance().publish(imp::protocol::MessageType::MemoryTelemetry, bytes);

			lastTelemetryPublish = now;
		}

		camera.update(window.input(), deltaTime);

		gfx::BlinnPhongLightUBO lightData;
		lightData.cameraPositionWS = { camera.position().x, camera.position().y, camera.position().z, 0.f };
		std::memcpy(lightBuffer->mappedData(), &lightData, sizeof(lightData));

		gfx::ICommandList* cmd = gfx->beginFrame();
		if (!cmd)
			continue;

		gfx::RenderPassDesc passDesc;
		passDesc.colourTarget = &gfx->backBuffer();
		passDesc.depthTarget = gfx->depthBuffer();
		passDesc.clearColourValue = { 0.023153f, 0.000911f, 0.004391f, 1.f };
		passDesc.clearDepthValue = 1.f;

		cmd->beginRenderPass(passDesc);
		cmd->bindPipeline(*pipeline);

		using namespace imp::math;
		const u32 w = gfx->backBuffer().width();
		const u32 h = gfx->backBuffer().height();
		const float aspect = h > 0 ? static_cast<float>( w ) / static_cast<float>( h ) : 1.f;

		Mat4f viewProj = camera.projection(aspect) * camera.view();
		Mat4f worldRoot{};

		std::function<void(u32, const Mat4f&)> drawNode = [&](u32 nodeIdx, const Mat4f& parentWorld)
			{
				const gfx::ModelNode& node = model.nodes[nodeIdx];
				Mat4f world = parentWorld * node.localTransform;

				if (node.meshIndex >= 0)
				{
					for (auto& prim : model.meshes[node.meshIndex].primitives)
					{
						gfx::MeshPushConstants pc;
						pc.model = world;
						pc.mvp = viewProj * world;
						cmd->pushConstants(&pc, sizeof(pc), 0);

						cmd->bindUniformBuffer(*lightBuffer, 0);

						if (prim.materialIndex >= 0)
						{
							i32 texIdx = model.materials[prim.materialIndex].baseColourTextureIndex;
							if (texIdx >= 0)
								cmd->bindTexture(*model.textures[texIdx].texture, *sampler, 1);
						}

						cmd->bindVertexBuffer(*prim.vertexBuffer);
						cmd->bindIndexBuffer(*prim.indexBuffer);
						cmd->drawIndexed(prim.indexCount, 1);
					}
				}

				for (u32 child : node.children)
					drawNode(child, world);
			};

		for (u32 root : model.rootNodes)
			drawNode(root, worldRoot);

		cmd->endRenderPass();
		gfx->endFrame();
	}
	model = gfx::Model{};
	lightBuffer.reset();
	sampler.reset();
	pipeline.reset();
	meshFragShader.reset();
	meshVertShader.reset();

	gfx->shutdown();
	window.destroy();

	imp::protocol::ToolServer::instance().stop();
	imp::log::Logger::get().shutdown();
	return 0;
}
