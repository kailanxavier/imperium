#include "perlingen/perlingen.h"
#include <string>

using namespace imp::tools::perlingen;

int main(int argc, char** argv)
{
	imp::log::Logger::get().initialise("tools.log");

	u32 width = 0;
	u32 height = 0;
	u32 seed = 0;
	float freq = 0.f;
	u32 octaves = 0;
	std::string output;

	for (u32 i = 1; i < static_cast<u32>(argc); ++i)
	{
		std::string arg = argv[i];

		if (arg == "-width" && i + 1 < static_cast<u32>(argc))
		{
			width = std::stoi(argv[++i]);
		}
		else if (arg == "-height" && i + 1 < static_cast<u32>(argc))
		{
			height = std::stoi(argv[++i]);
		}
		else if (arg == "-seed" && i + 1 < static_cast<u32>(argc))
		{
			seed = std::stoi(argv[++i]);
		}
		else if (arg == "-freq" && i + 1 < static_cast<u32>(argc))
		{
			freq = std::stof(argv[++i]);
		}
		else if (arg == "-octaves" && i + 1 < static_cast<u32>(argc))
		{
			octaves = std::stoi(argv[++i]);
		}
		else if (arg == "-output" && i + 1 < static_cast<u32>(argc))
		{
			output = argv[++i];
		}
	}

	PerlinGenerator gen(width, height, seed, freq, octaves, output);
	gen.write();

	imp::log::Logger::get().shutdown();

	return 0;
}
