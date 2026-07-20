#include <core/core.h>
#include <core/log/log.h>
#include <core/memory/heap_allocator.h>
#include <fwk/window.h>
#include <gfx/gfx.h>

#include <memory>

#include <core/math/math.h>

#include <core/fs/vfs.h>
#include <core/platform/exe_path.h>

#include <protocol/tool_server.h>
#include <protocol/memory_telemetry.h>

using namespace imp;

namespace
{
	struct Vertex
	{
		math::Vec3f position;
		math::Vec3f colour;
	};

	const Vertex kCubeVertices[] = {
		{ { -0.5f, -0.5f, -0.5f }, { 1.f,  0.f,  0.f } },
		{ {  0.5f, -0.5f, -0.5f }, { 0.f,  1.f,  0.f } },
		{ {  0.5f,  0.5f, -0.5f }, { 0.f,  0.f,  1.f } },
		{ { -0.5f,  0.5f, -0.5f }, { 1.f,  1.f,  0.f } },
		{ { -0.5f, -0.5f,  0.5f }, { 1.f,  0.f,  1.f } },
		{ {  0.5f, -0.5f,  0.5f }, { 0.f,  1.f,  1.f } },
		{ {  0.5f,  0.5f,  0.5f }, { 1.f,  1.f,  1.f } },
		{ { -0.5f,  0.5f,  0.5f }, { 0.5f, 0.5f, 0.5f} }
	};

	const u16 kCubeIndices[] = {
		0, 2, 1,  0, 3, 2,
		4, 5, 6,  4, 6, 7,
		0, 7, 3,  0, 4, 7,
		1, 2, 6,  1, 6, 5,
		0, 1, 5,  0, 5, 4,
		3, 6, 2,  3, 7, 6,
	};
}

int main()
{
	log::Logger::get().initialise();
	protocol::ToolServer::instance().start(47810);

	LOG_INFO("Sandbox", "App starting...");
	
	memory::HeapAllocator gfxHostAllocator("GfxHost");
	fs::VirtualFileSystem vfsHost;

	const auto shadersPath = (imp::platform::executableDir() / "shaders").string();
	if (!vfsHost.mount("shaders/", shadersPath, 0, true, true))
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

	// RESOURCE CREATION WAS MOVED HERE
	gfx::ShaderDesc vertDesc;
	vertDesc.stage = gfx::ShaderStage::Vertex;
	vertDesc.path = "shaders/triangle.vert.spv";
	std::unique_ptr<gfx::IShader> vertexShader = gfx->createShader(vertDesc);

	gfx::ShaderDesc fragDesc;
	fragDesc.stage = gfx::ShaderStage::Fragment;
	fragDesc.path = "shaders/triangle.frag.spv";
	std::unique_ptr<gfx::IShader> fragmentShader = gfx->createShader(fragDesc);

	if (!vertexShader || !fragmentShader)
	{
		LOG_FATAL("Sandbox", "Failed to load shaders");
		gfx->shutdown();
		window.destroy();
		return 1;
	}

	gfx::VertexAttribute attrs[2] = {
		{ 0, static_cast<uint32_t>(offsetof(Vertex, position)), 3, true },
		{ 1, static_cast<uint32_t>(offsetof(Vertex, colour)), 3, true },
	};

	gfx::PipelineDesc pipelineDesc;
	pipelineDesc.vertexShader = vertexShader.get();
	pipelineDesc.fragmentShader = fragmentShader.get();
	pipelineDesc.vertexLayout.stride = sizeof(Vertex);
	pipelineDesc.vertexLayout.attributes = attrs;
	pipelineDesc.vertexLayout.attributeCount = 2;
	pipelineDesc.rasterizerState.cullMode = gfx::CullMode::Back;
	pipelineDesc.depthStencilState.depthTestEnable = true;
	pipelineDesc.depthStencilState.depthWriteEnable = true;
	pipelineDesc.depthStencilState.depthCompareOp = gfx::CompareOp::Less;
	pipelineDesc.colourFormat = gfx->backBuffer().format();
	pipelineDesc.depthFormat = gfx->depthBuffer() ? gfx->depthBuffer()->format() : gfx::TextureFormat::Unknown;
	pipelineDesc.pushConstantSize = sizeof(math::Mat4f);

	std::unique_ptr<gfx::IPipeline> pipeline = gfx->createPipeline(pipelineDesc);

	gfx::BufferDesc vbDesc;
	vbDesc.size = sizeof(kCubeVertices);
	vbDesc.usage = gfx::BufferUsage::Vertex;
	vbDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
	std::unique_ptr<gfx::IBuffer> vertexBuffer = gfx->createBuffer(vbDesc);

	gfx::BufferDesc ibDesc;
	ibDesc.size = sizeof(kCubeIndices);
	ibDesc.usage = gfx::BufferUsage::Index;
	ibDesc.memoryAccess = gfx::MemoryAccess::HostVisible;
	std::unique_ptr<gfx::IBuffer> indexBuffer = gfx->createBuffer(ibDesc);

	if (!pipeline || !vertexBuffer || !indexBuffer)
	{
		LOG_FATAL("Sandbox", "Failed to create pipeline/buffers");
		gfx->shutdown();
		window.destroy();
		return 1;
	}

	std::memcpy(vertexBuffer->mappedData(), kCubeVertices, sizeof(kCubeVertices));
	std::memcpy(indexBuffer->mappedData(), kCubeIndices, sizeof(kCubeIndices));

	// Tool server stuff
	auto lastTelemetryPublish = std::chrono::steady_clock::now();
	constexpr auto kTelemetryInterval = std::chrono::milliseconds(200);

	float rotationAngle = 0.f; // temp

	while (!window.shouldClose())
	{
		window.pollEvents();

		if (window.isMinimised())
			continue;

		const auto now = std::chrono::steady_clock::now();
		if (now - lastTelemetryPublish >= kTelemetryInterval &&
			imp::protocol::ToolServer::instance().hasSubscribers(imp::protocol::MessageType::MemoryTelemetry))
		{
			const auto snap = gfxHostAllocator.statsSnapshot();

			imp::protocol::AllocatorStatsPayload payload;
			payload.name            = std::string(gfxHostAllocator.name());
			payload.totalAllocated  = snap.totalAllocated;
			payload.totalFreed      = snap.totalFreed;
			payload.currentUsed     = snap.currentUsed;
			payload.peakUsed        = snap.peakUsed;
			payload.allocationCount = snap.allocationCount;
			payload.freeCount       = snap.freeCount;
			payload.tagBytes.assign(std::begin(snap.tagBytes), std::end(snap.tagBytes));

			const auto bytes = imp::protocol::serialiseMemoryTelemetry({payload});
			imp::protocol::ToolServer::instance().publish(imp::protocol::MessageType::MemoryTelemetry, bytes);

			lastTelemetryPublish = now;
		}

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

		Mat4f model = makeRotationY(rotationAngle);
		Mat4f view = makeLookAtLH(Vec3f::unitZ() * -1.5f, Vec3f::zero(), Vec3f::up());
		Mat4f proj = makePerspectiveLH(toRadians(90.f), aspect, 0.1f, 100.f);
		Mat4f mvp = proj * view * model;

		cmd->pushConstants(mvp.data(), sizeof(Mat4f));
		cmd->bindVertexBuffer(*vertexBuffer);
		cmd->bindIndexBuffer(*indexBuffer);
		cmd->drawIndexed(static_cast<u32>( std::size(kCubeIndices) ));

		cmd->endRenderPass();
		gfx->endFrame();

		rotationAngle += 0.01f;
	}
	indexBuffer.reset();
	vertexBuffer.reset();
	pipeline.reset();
	fragmentShader.reset();
	vertexShader.reset();

	gfx->shutdown();
	window.destroy();

	imp::protocol::ToolServer::instance().stop();
	imp::log::Logger::get().shutdown();
	return 0;
}
