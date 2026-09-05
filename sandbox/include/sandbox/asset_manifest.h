#pragma once
#include <string>

namespace imp::app
{
	struct AssetManifest
	{
		std::string meshVertShader = "assets/shaders/mesh.vert.spv";
		std::string meshFragShader = "assets/shaders/mesh.frag.spv";
		std::string shadowVertShader = "assets/shaders/shadow.vert.spv";
		std::string shadowFragShader = "assets/shaders/shadow.frag.spv";
		std::string tonemapVertShader = "assets/shaders/tonemap.vert.spv";
		std::string tonemapFragShader = "assets/shaders/tonemap.frag.spv";
		std::string skyVertShader = "assets/shaders/sky.vert.spv";
		std::string skyFragShader = "assets/shaders/sky.frag.spv";
		std::string prepassVertShader = "assets/shaders/depth_normal_prepass.vert.spv";
		std::string prepassFragShader = "assets/shaders/depth_normal_prepass.frag.spv";
		std::string fullscreenVertShader = "assets/shaders/fullscreen.vert.spv";
		std::string gtaoFragShader = "assets/shaders/gtao.frag.spv";
		std::string blurFragShader = "assets/shaders/bilateral_blur.frag.spv";
		std::string ddgiProbeUpdateShader = "assets/shaders/ddgi_probe_update.comp.spv";

		std::string environmentModel = "assets/models/khr-sponza.glb";
		std::string environmentTestModel = "assets/models/environment_test.glb";
	};
}
