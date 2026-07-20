#if defined(IMP_GFX_DX11)
#include "dx11_device.h"
#include <core/log/log.h>
#include <cstdlib>

namespace imp::gfx::dx11
{
	bool DX11Device::initialise(const gfx::DeviceDesc& desc)
	{
		LOG_ERROR("D3D11", "Backend not implemented yet");
		return false;
	}

	void DX11Device::shutdown() { m_initialised = false; }

	std::unique_ptr<gfx::IBuffer> DX11Device::createBuffer(const gfx::BufferDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::ITexture> DX11Device::createTexture(const gfx::TextureDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::ISampler> DX11Device::createSampler(const gfx::SamplerDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::IShader> DX11Device::createShader(const gfx::ShaderDesc& desc) { return nullptr; }
	std::unique_ptr<gfx::IPipeline> DX11Device::createPipeline(const gfx::PipelineDesc& desc) { return nullptr; }

	gfx::IRenderTarget& DX11Device::backBuffer() { std::abort(); }
	gfx::IRenderTarget* DX11Device::depthBuffer() { return nullptr; }
	gfx::ICommandList* DX11Device::beginFrame() { return nullptr; }

	bool DX11Device::initImGui() { return false; }
	void DX11Device::shutdownImGui() {}
	void DX11Device::newImGuiFrame() {}
	void DX11Device::renderImGui(ICommandList& cmd) {}

	void DX11Device::endFrame() {}
}

#endif
