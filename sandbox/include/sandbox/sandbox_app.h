#pragma once

#include <app/iapp.h>
#include <camera/camera.h>

#include <gfx/model.h>
#include <gfx/model_registry.h>

#include <core/types/int_types.h>
#include <core/math/math.h>

#include <ecs/world.h>

#include <app/render_extraction.h>
#include <gfx/texture_cache.h>

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
		void drawNode(gfx::ICommandList& cmd, gfx::Model& model, const math::Mat4f& viewProj, u32 nodeIdx, const math::Mat4f& parentWorld);

		void ensureInstanceBufferCapacity(AppContext& ctx, u32 instanceCount);
		ecs::EntityId spawnInstance(AppContext& ctx, const ecs::Transform& t);

		fwk::Camera m_camera;

		std::unique_ptr<gfx::IShader> m_meshVertShader;
		std::unique_ptr<gfx::IShader> m_meshFragShader;
		std::unique_ptr<gfx::IPipeline> m_pipeline;
		std::unique_ptr<gfx::ISampler> m_sampler;
		std::unique_ptr<gfx::IBuffer> m_lightBuffer;
		std::unique_ptr<gfx::IBuffer> m_instanceBuffer;

		/*gfx::Model m_model;
		gfx::Model m_statue;*/

		u32 m_instanceCapacity = 16;

		std::vector<ecs::EntityId> m_instances;
		gfx::ModelRegistry m_modelRegistry;

		ecs::ModelHandle m_environmentHandle{};
		ecs::ModelHandle m_statueHandle{};

		RenderExtraction m_extraction;
	};
}
