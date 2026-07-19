#pragma once

#include <gfx/device.h>

namespace imp::fs { class VirtualFileSystem; }
namespace imp::gfx::dx12
{
	class DX12Device final : public gfx::IDevice
	{
	public:
		bool initialise(const gfx::DeviceDesc& desc) override;
		void shutdown() override;

		[[nodiscard]] std::unique_ptr<gfx::IBuffer> createBuffer(const gfx::BufferDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::ITexture> createTexture(const gfx::TextureDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::ISampler> createSampler(const gfx::SamplerDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IShader> createShader(const gfx::ShaderDesc& desc) override;
		[[nodiscard]] std::unique_ptr<gfx::IPipeline> createPipeline(const gfx::PipelineDesc& desc) override;

		gfx::IRenderTarget& backBuffer() override;
		gfx::IRenderTarget* depthBuffer() override;

		gfx::ICommandList* beginFrame() override;
		void endFrame() override;

		gfx::GraphicsApi api() const override { return gfx::GraphicsApi::D3D12; }
		const char* apiName() const override { return "DirectX 12"; }

	private:
		bool m_initialised = false;
		const fs::VirtualFileSystem* m_vfs = nullptr;
	};
}
