#pragma once

#include <app/iapp.h>
#include <camera/camera.h>
#include <gfx/model.h>
#include <core/memory/int_types.h>
#include <core/math/math.h>

#include <memory>

namespace imp::gfx
{
	class IShader;
	class IPipeline;
	class ISampler;
	class IBuffer;
}

namespace imp::app
{
	class SandboxApp final : public IApp
	{
	public:
		bool onInit(AppContext& ctx) override;
		void onUpdate(AppContext& ctx, float deltaSeconds) override;
		void onRender(AppContext& ctx, gfx::ICommandList& cmd) override;
		void onShutdown(AppContext& ctx) override;

	private:
		void drawNode(gfx::ICommandList& cmd, const math::Mat4f& viewProj, u32 nodeIdx, const math::Mat4f& parentWorld);
		fwk::Camera m_camera;

		std::unique_ptr<gfx::IShader> m_meshVertShader;
		std::unique_ptr<gfx::IShader> m_meshFragShader;
		std::unique_ptr<gfx::IPipeline> m_pipeline;
		std::unique_ptr<gfx::ISampler> m_sampler;
		std::unique_ptr<gfx::IBuffer> m_lightBuffer;
		gfx::Model m_model;
	};
}
