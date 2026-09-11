#version 450

layout(binding = 1) uniform sampler2D ddgiIrradianceAtlas;

layout(push_constant) uniform PushConstants
{
	mat4 viewProj;
	vec4 cameraForwardAndRadius;
	vec4 minCornerAndSpacing;
	uint probeCountX;
	uint probeCountY;
	uint probeCountZ;
	uint showInactive;
	uint activeWindowOffset;
	uint activeWindowCount;
} pc;

layout(location = 0) in vec2 inLocalUV;
layout(location = 1) in flat uint inProbeIndex;
layout(location = 2) in flat float inActive;
layout(location = 3) in flat float inInActiveWindow;

layout(location = 0) out vec4 outColour;

// these have to stay the same as the ones in ddgi_volume.h
const uint kDDGIIrradianceInteriorTexels = 6u;
const uint kDDGIIrradianceTileTexels = kDDGIIrradianceInteriorTexels + 2u;

vec2 octEncode(vec3 n)
{
	vec2 p = n.xy * (1.0 / (abs(n.x) + abs(n.y) + abs(n.z)));
	return (n.z <= 0.0) ? ((1.0 - abs(p.yx)) * vec2(p.x >= 0.0 ? 1.0 : -1.0, p.y >= 0.0 ? 1.0 : -1.0)) : p;
}

vec3 sampleIrradiance(uvec2 atlasProbeCoord, vec3 dir)
{
	vec2 oct = octEncode(dir) * 0.5 + 0.5;
	oct = clamp(oct, vec2(0.5 / float(kDDGIIrradianceInteriorTexels)), 
					vec2(1.0 - 0.5 / float(kDDGIIrradianceInteriorTexels)));

	vec2 texel = vec2(atlasProbeCoord) * float(kDDGIIrradianceTileTexels) + 1.0 + oct * float(kDDGIIrradianceInteriorTexels);
	vec2 atlasSize = vec2(textureSize(ddgiIrradianceAtlas, 0));
	return textureLod(ddgiIrradianceAtlas, texel / atlasSize, 0.0).rgb;
}

void main()
{
	float r2 = dot(inLocalUV, inLocalUV);
	if (r2 > 1.0)
		discard;

	float z = sqrt(1.0 - r2);

	float windowRing = 0.0;
	if (inInActiveWindow > 0.5)
	{
		float ringBand = smoothstep(0.78, 0.86, z) - smoothstep(0.86, 0.94, z);
		windowRing = clamp(ringBand, 0.0, 1.0);
	}

	if (inActive < 0.5)
	{
		float shade = mix(0.25, 0.55, z);
		vec3 colour = vec3(shade, 0.03, 0.03);
		colour = mix(colour, vec3(1.0, 1.0, 1.0), windowRing * 0.6);
		outColour = vec4(colour, 1.0);
		return;
	}

	vec3 forward = normalize(pc.cameraForwardAndRadius.xyz);
	vec3 worldUp = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(worldUp, forward));
	vec3 up = cross(forward, right);

	vec3 normal = normalize(inLocalUV.x * right + inLocalUV.y * up - forward * z);

	uint tileCol = inProbeIndex % (pc.probeCountX * pc.probeCountY);
	uint tileRow = inProbeIndex / (pc.probeCountX * pc.probeCountY);

	vec3 emitted = sampleIrradiance(uvec2(tileCol, tileRow), normal);

	float rim = pow(1.0 - z, 2.0);
	outColour = vec4(emitted + rim * 0.05, 1.0);
}
