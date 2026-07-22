#include "perlingen/perlingen.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <perlingen/stb_image_write.h>

namespace imp::tools::perlingen
{
	PerlinGenerator::PerlinGenerator(u32 width, u32 height, u32 seed, float freq, u32 octaves, std::string output)
	{
		if (width > 0) m_width = width;
		if (height > 0) m_height = height;
		if (seed > 0) m_seed = seed;
		if (freq > 0) m_freq = freq;
		if (octaves > 0) m_octaves = octaves;
		if (!output.empty()) m_path = output;
	}

	float PerlinGenerator::perlin(float x, float y)
	{
		int x0 = static_cast<int>( x );
		int y0 = static_cast<int>( y );
		int x1 = x0 + 1;
		int y1 = y0 + 1;

		float sx = x - static_cast<float>( x0 );
		float sy = y - static_cast<float>( y0 );

		float n0 = dotGridGradient(x0, y0, x, y);
		float n1 = dotGridGradient(x1, y0, x, y);
		float ix0 = interpolate(n0, n1, sx);

		n0 = dotGridGradient(x0, y1, x, y);
		n1 = dotGridGradient(x1, y1, x, y);
		float ix1 = interpolate(n0, n1, sx);

		float value = interpolate(ix0, ix1, sy);

		return value;
	}

	math::Vec2f PerlinGenerator::randomGradient(u32 ix, u32 iy)
	{
		const u32 w = 8 * sizeof(u32);
		const u32 s = w / 2;
		u32 a = ix, b = iy;
		a *= 3284157443;

		b ^= ( a << s ) | ( a >> ( w - s ) );
		b *= 1911520717;

		a ^= ( b << s ) | ( b >> ( w - s ) );
		a *= 2048419325;

		float random = static_cast<float>( a ) / static_cast<float>( UINT32_MAX );
		random *= math::kPif * 2.f;

		math::Vec2f v(sin(random), cos(random));
		return v;
	}

	float PerlinGenerator::dotGridGradient(u32 ix, u32 iy, float x, float y)
	{
		math::Vec2f gradient = randomGradient(ix, iy);

		float dx = x - static_cast<float>( ix );
		float dy = y - static_cast<float>( iy );

		return ( dx * gradient.x + dy * gradient.y );
	}

	float PerlinGenerator::interpolate(float a0, float a1, float w)
	{
		return ( a1 - a0 ) * ( 3.f - w * 2.f ) * w * w + a0;
	}

	void PerlinGenerator::generate()
	{
		m_pixels.resize(static_cast<size_t>(m_width * m_height * 4));
		for (u32 x = 0; x < m_width; ++x)
		{
			for (u32 y = 0; y < m_height; ++y)
			{
				u32 index = ( y * m_width + x ) * 4;

				float val = 0.f;

				float freq = m_freq;
				float amp = 1.f;

				for (u32 i = 0; i < m_octaves; ++i)
				{
					val += perlin(x * freq / kGridSize, y * freq / kGridSize) * amp;
					freq *= 2.f;
					amp /= 2.f;
				}

				// Contrast
				val *= 1.2f;

				if (val > 1.f) val = 1.f;
				else if (val < -1.f) val = -1.f;

				u8 colour = static_cast<u8>(( ( val + 1.f ) * 0.5f ) * 255);

				m_pixels[static_cast<size_t>(index)] = colour;
				m_pixels[static_cast<size_t>(index) + 1] = colour;
				m_pixels[static_cast<size_t>(index) + 2] = colour;
				m_pixels[static_cast<size_t>(index) + 3] = 255;
			}
		}
	}

	void PerlinGenerator::write()
	{
		generate();
		if (!stbi_write_png(m_path.c_str(), m_width, m_height, 4, m_pixels.data(), m_width * 4))
		{
			LOG_ERROR("Perlin Generator", "stbi_write_png failed");
			return;
		}

		LOG_INFO("Perlin Generator", "Wrote image to: {}", m_path.c_str());
	}

}
