#pragma once
#include <core/math/math.h>
#include <core/log/log.h>
#include <core/memory/int_types.h>
#include <string>
#include <vector>

namespace imp::tools::perlingen
{
	class PerlinGenerator
	{
	public:
		PerlinGenerator(u32 width, u32 height, u32 seed, float freq = 0.02, u32 octaves = 12, std::string output = "perlin.png");
		~PerlinGenerator() = default;

		void generate();
		void write();

		[[nodiscard]] u32 width() const { return m_width; }
		[[nodiscard]] u32 height() const { return m_height; }

		[[nodiscard]] std::vector<u8> pixels() const { return m_pixels; }

	private:
		float perlin(float x, float y);
		math::Vec2f randomGradient(u32 ix, u32 iy);
		float dotGridGradient(u32 ix, u32 iy, float x, float y);
		float interpolate(float a0, float a1, float w);

		u32 m_width = 512;
		u32 m_height = 512;

		static constexpr float kGridSize = 200.f;

		u32 m_seed = 42;
		float m_freq = 0.02f;
		u32 m_octaves = 12;
		std::string m_path;

		std::vector<u8> m_pixels;
	};
}
