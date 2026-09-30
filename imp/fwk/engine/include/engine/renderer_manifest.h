#pragma once
#include <string>

namespace imp::engine
{
	struct RendererManifest
	{
		std::string meshVertShader = "engine/shaders/mesh.vert.spv";
		std::string meshFragShader = "engine/shaders/mesh.frag.spv";
		std::string shadowVertShader = "engine/shaders/shadow.vert.spv";
		std::string shadowFragShader = "engine/shaders/shadow.frag.spv";
		std::string tonemapVertShader = "engine/shaders/tonemap.vert.spv";
		std::string tonemapFragShader = "engine/shaders/tonemap.frag.spv";
		std::string skyVertShader = "engine/shaders/sky.vert.spv";
		std::string skyFragShader = "engine/shaders/sky.frag.spv";
		std::string prepassVertShader = "engine/shaders/depth_normal_prepass.vert.spv";
		std::string prepassFragShader = "engine/shaders/depth_normal_prepass.frag.spv";
		std::string fullscreenVertShader = "engine/shaders/fullscreen.vert.spv";
		std::string gtaoFragShader = "engine/shaders/gtao.frag.spv";
		std::string blurFragShader = "engine/shaders/bilateral_blur.frag.spv";
		std::string gbufferDebugFragShader = "engine/shaders/gbuffer_debug.frag.spv";
		std::string deferredLightingFragShader = "engine/shaders/deferred_lighting.frag.spv";
		std::string taaResolveFragShader = "engine/shaders/taa_resolve.frag.spv";
		std::string ssgiFragShader = "engine/shaders/ssgi.frag.spv";
		std::string ssgiBlurFragShader = "engine/shaders/ssgi_blur.frag.spv";

		std::string ddgiProbeUpdateShader = "engine/shaders/ddgi_probe_update.comp.spv";
		std::string ddgiRayTraceShader = "engine/shaders/ddgi_ray_trace.comp.spv";
		std::string ddgiClassifyShader = "engine/shaders/ddgi_classify_probes.comp.spv";
		std::string bloomDownsampleFragShader = "engine/shaders/bloom_down_sample.frag.spv";
		std::string bloomUpsampleFragShader = "engine/shaders/bloom_up_sample.frag.spv";
		std::string thermalUpdateDdgiShader = "engine/shaders/ddgi_thermal_update.comp.spv";
		std::string thermalUpdateFallbackShader = "engine/shaders/thermal_update_fallback.comp.spv";

		std::string ddgiDebugProbesVertShader = "engine/shaders/ddgi_debug_probes.vert.spv";
		std::string ddgiDebugProbesFragShader = "engine/shaders/ddgi_debug_probes.frag.spv";
		std::string ddgiDebugRaysVertShader = "engine/shaders/ddgi_debug_rays.vert.spv";
		std::string ddgiDebugRaysFragShader = "engine/shaders/ddgi_debug_rays.frag.spv";
	};
}
