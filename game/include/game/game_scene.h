#pragma once

#include <app/iapp.h>
#include <gfx/model_registry.h>

#include <string>

namespace imp::game
{
	using app::AppContext;

	bool loadStartupScene(AppContext& ctx, gfx::ModelRegistry& models, const std::string& path);
	void unloadScene(AppContext& ctx);
}
