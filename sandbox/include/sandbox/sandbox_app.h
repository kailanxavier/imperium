#pragma once

#include <app/iapp.h>
#include <camera/camera.h>
#include <sandbox/asset_manifest.h>
#include <sandbox/render_resources.h>
#include <sandbox/sandbox_scene.h>

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

		math::Vec3f& sunDirection() { return m_scene.sunDirection(); }
		const math::Vec3f& sunDirection() const { return m_scene.sunDirection(); }
		gfx::CascadeConfig& cascadeConfig() { return m_scene.cascadeConfig(); }
		ecs::Transform& pointPos() { return m_scene.pointLightTransform(); }
		const ecs::Transform& pointPos() const { return m_scene.pointLightTransform(); }

		const fwk::Camera& camera() const { return m_camera; }
		gfx::TextureFormat hdrColourFormat() const { return m_resources.hdrColourFormat(); }
		gfx::TextureFormat hdrDepthFormat() const { return m_resources.hdrDepthFormat(); }
		gfx::SampleCount sampleCount() const { return RenderResources::kMsaaSampleCount; }

	private:
		fwk::Camera m_camera;
		AssetManifest m_assets;
		RenderResources m_resources;
		SandboxScene m_scene;
		bool m_enableFrustumCulling = true;
	};
}
