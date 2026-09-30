#version 450

layout(location = 0) in vec3 inViewDirWS;
layout(location = 0) out vec4 outColour;

layout(binding = 0) uniform SkyUBO
{
	vec4 sunDirAndIntensity;
	vec4 sunColour;
	vec4 moonDirAndIntensity;
	vec4 moonColour;
	vec4 rayleighAndMieG;
	vec4 scatterParams;
	vec4 groundAndStars;
} sky;

const float PI = 3.14159265359;

const float kSunDiscStrength = 3.0;
const float kMoonDiscStrength = 6.0;
const vec3  kStarTint = vec3(0.85, 0.9, 1.0);

vec3 scatterFrom(vec3 viewDir, vec3 bodyDir, vec3 radiance)
{
	float cosTheta = dot(viewDir, bodyDir);
	float elevation = max(viewDir.y, 0.001); // avoid divide blowup right at the horizon

	float g = sky.rayleighAndMieG.w;
	float phaseR = (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
	float phaseM = (1.0 - g * g) / (4.0 * PI * pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5));

	// longer path length near the horizon -> more scattering
	float opticalDepth = 1.0 / elevation;

	vec3 rayleigh = sky.rayleighAndMieG.xyz * phaseR * opticalDepth;
	float mie = sky.scatterParams.x * phaseM * opticalDepth;

	return (rayleigh + vec3(mie)) * radiance * sky.scatterParams.y;
}

vec3 discFrom(vec3 viewDir, vec3 bodyDir, float angularRadius, vec3 radiance, float strength)
{
	float edge = cos(angularRadius);
	float disc = smoothstep(edge, edge + 0.0006, dot(viewDir, bodyDir));
	return disc * radiance * strength;
}

float hash13(vec3 p3)
{
	p3 = fract(p3 * 0.1031);
	p3 += dot(p3, p3.xyz + 31.32);
	return fract((p3.x + p3.y) * p3.z);
}

float starField(vec3 dir)
{
	vec3 p = dir * 110.0;
	vec3 cell = floor(p);
	vec3 local = fract(p) - 0.5;

	if (hash13(cell) < 0.965)
		return 0.0;

	vec3 offset = vec3(hash13(cell + 17.3), hash13(cell + 41.7), hash13(cell + 83.1)) - 0.5;
	float d = length(local - offset * 0.6);

	float size = mix(0.1, 0.22, hash13(cell + 5.5));
	float brightness = mix(0.4, 2.0, hash13(cell + 9.1));

	return (1.0 - smoothstep(0.0, size, d)) * brightness;
}

void main()
{
	vec3 viewDir = normalize(inViewDirWS);
	vec3 sunDir = normalize(sky.sunDirAndIntensity.xyz);
	vec3 moonDir = normalize(sky.moonDirAndIntensity.xyz);
	vec3 sunRadiance = sky.sunColour.rgb * sky.sunDirAndIntensity.w;
	vec3 moonRadiance = sky.moonColour.rgb * sky.moonDirAndIntensity.w;

	vec3 colour = scatterFrom(viewDir, sunDir, sunRadiance) 
		+ scatterFrom(viewDir, moonDir, moonRadiance);

	colour += discFrom(viewDir, sunDir, sky.scatterParams.z, sunRadiance, kSunDiscStrength);
	colour += discFrom(viewDir, moonDir, sky.scatterParams.w, moonRadiance, kMoonDiscStrength);

	float starFade = smoothstep(0.02, 0.2, viewDir.y);
	colour += kStarTint * starField(viewDir) * starFade * sky.groundAndStars.w * 2.0;

	if (viewDir.y < 0.0)
		colour = mix(colour, sky.groundAndStars.rgb, clamp(-viewDir.y * 4.0, 0.0, 1.0));

	outColour = vec4(colour, 1.0);
}
