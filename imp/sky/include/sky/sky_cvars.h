#pragma once

#include <sky/sky_settings.h>
#include <vector>

namespace imp::sky
{
	class SkyCVars
	{
	public:
		SkyCVars();

		void pull(SkySettings& settings);
		void push(const SkySettings& settings);

	private:
		struct FloatBinding
		{
			float& ( *field )( SkySettings& ) = nullptr;
			float* cvar = nullptr;
			float last = 0.f;
		};

		struct BoolBinding
		{
			bool& ( *field )( SkySettings& ) = nullptr;
			bool* cvar = nullptr;
			bool last = false;
		};

		std::vector<FloatBinding> m_floats;
		std::vector<BoolBinding> m_bools;
	};
}
