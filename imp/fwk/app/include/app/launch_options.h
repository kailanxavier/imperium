#pragma once

#include <gfx/device.h>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace imp::app
{
	struct ApplicationDesc;

	struct LaunchOptions
	{
		bool editor = kDefaultLaunchesEditor;
		bool useOwnWindow = false;
		std::optional<gfx::GraphicsApi> graphicsApi;

		std::unordered_map<std::string, std::string> args;

		static constexpr bool kDefaultLaunchesEditor = false;

		[[nodiscard]] bool has(std::string_view key) const;
		[[nodiscard]] std::optional<std::string> get(std::string_view key) const;

		void applyTo(ApplicationDesc& desc) const;
	};

	[[nodiscard]] LaunchOptions parseLaunchOptions(int argc, const char* const* argv);
}
