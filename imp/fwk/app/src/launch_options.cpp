#include <app/launch_options.h>
#include <app/application.h>

#include <core/log/log.h>

#include <string_view>

namespace imp::app
{
	namespace
	{
		bool isFlag(std::string_view token)
		{
			return token.size() > 1 && token.front() == '-';
		}

		std::string_view stripDashes(std::string_view token)
		{
			token.remove_prefix(token.size() > 1 && token[1] == '-' ? 2 : 1);
			return token;
		}

		void setApi(LaunchOptions& out, gfx::GraphicsApi api, std::string_view flag)
		{
			if (out.graphicsApi && *out.graphicsApi != api)
				LOG_WARN("Launcher", "Multiple graphics API flags given. '{}' wins", flag);
			out.graphicsApi = api;
		}
	}

	bool LaunchOptions::has(std::string_view key) const
	{
		return args.contains(std::string(key));
	}

	std::optional<std::string> LaunchOptions::get(std::string_view key) const
	{
		const auto it = args.find(std::string(key));
		if (it == args.end())
			return std::nullopt;
		return it->second;
	}

	void LaunchOptions::applyTo(ApplicationDesc& desc) const
	{
		if (!graphicsApi) return;

		desc.autoSelectGraphicsApi = false;
		desc.graphicsApi = *graphicsApi;
	}

	LaunchOptions parseLaunchOptions(int argc, const char* const* argv)
	{
		LaunchOptions out;
		bool sawImp = false;
		bool sawOwnWindow = false;

		for (int i{ 1 }; i < argc; ++i)
		{
			const std::string_view token = argv[i];
			if (!isFlag(token))
			{
				LOG_WARN("Launcher", "Ignoring stray argument '{}'", token);
				continue;
			}

			const std::string_view key = stripDashes(token);

			if (key == "imp") { out.editor = true; sawImp = true; }
			else if (key == "impUseOwnWindow") { out.useOwnWindow = true; sawOwnWindow = true; }
			else if (key == "vulkan") { setApi(out, gfx::GraphicsApi::Vulkan, token); }
			else if (key == "dx11") { setApi(out, gfx::GraphicsApi::D3D11, token); }
			else if (key == "dx12") { setApi(out, gfx::GraphicsApi::D3D12, token); }
			else
			{
				std::string value;
				if (i + 1 < argc && !isFlag(argv[i + 1]))
					value = argv[++i];
				out.args[std::string(key)] = std::move(value);
			}
		}

		if (sawOwnWindow && !sawImp)
			out.editor = false;

		return out;
	}
}
