#if defined(IMP_GFX_DX12)
#include "dx12_device.h"
#include <core/log/log.h>
#include <cstdlib>

namespace imp::gfx::dx12
{
	bool DX12Device::initialise(const gfx::DeviceDesc& desc)
	{
		LOG_ERROR("D3D12", "Backend not implemented yet");
		return false;
	}

	void DX12Device::shutdown() { m_initialised = false; }

	std::unique_ptr<gfx::IBuffer> DX12Device::createBuffer(const gfx::BufferDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::ITexture> DX12Device::createTexture(const gfx::TextureDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::ISampler> DX12Device::createSampler(const gfx::SamplerDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::IShader> DX12Device::createShader(const gfx::ShaderDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::IPipeline> DX12Device::createPipeline(const gfx::PipelineDesc& desc) { return nullptr; }

	gfx::IRenderTarget& DX12Device::backBuffer() { std::abort(); }
	gfx::IRenderTarget* DX12Device::depthBuffer() { return nullptr; }

	bool DX12Device::initImGui() { return false; }
	void DX12Device::shutdownImGui() {}
	void DX12Device::newImGuiFrame() {}
	void DX12Device::renderImGui(ICommandList& cmd) {}

	gfx::ICommandList* DX12Device::beginFrame() { return nullptr; }

	void DX12Device::endFrame() {}

}

#endif
