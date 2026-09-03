#version 450

layout(location = 0) in vec3 inViewDirWS;
layout(location = 1) in vec4 inSunDirAndIntensity;
layout(location = 0) out vec4 outColour;

const float PI = 3.14159265359;

const vec3  kRayleighCoeff = vec3(12.0, 8.0, 22.0);
const float kMieCoeff = 55.0;
const float kMieG = 0.82;
const float kSunAngularRadius = radians(2.13);
const vec3  kGroundColour = vec3(0.055, 0.004, 0.003);

vec3 computeSky(vec3 viewDir, vec3 sunDir, float sunIntensity)
{
	viewDir = normalize(viewDir);
	sunDir = normalize(sunDir);

	float cosTheta = dot(viewDir, sunDir);
	float elevation = max(viewDir.y, 0.001); // avoid divide blowup right at the horizon

	float phaseR = (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
	float phaseM = (1.0 - kMieG * kMieG)
		/ (4.0 * PI * pow(1.0 + kMieG * kMieG - 2.0 * kMieG * cosTheta, 1.5));

	// longer path length near the horizon -> more scattering
	float opticalDepth = 1.0 / elevation;

	vec3 rayleigh = kRayleighCoeff * phaseR * opticalDepth;
	float mie = kMieCoeff * phaseM * opticalDepth;

	vec3 sunColour = vec3(1.0, 0.95, 0.85) * sunIntensity;
	vec3 scattered = (rayleigh + vec3(mie)) * sunColour * 0.0008;

	// sun disc, additive, bright enough to bloom later
	float sunEdge = cos(kSunAngularRadius);
	float sunDisc = smoothstep(sunEdge, sunEdge + 0.0006, cosTheta);
	scattered += sunDisc * sunColour * 3.0;

	return scattered;
}

void main()
{
	vec3 viewDir = normalize(inViewDirWS);
	vec3 sunDir = inSunDirAndIntensity.xyz;
	float sunIntensity = inSunDirAndIntensity.w;

	vec3 colour = computeSky(viewDir, sunDir, sunIntensity);

	if (viewDir.y < 0.0)
		colour = mix(colour, kGroundColour, clamp(-viewDir.y * 4.0, 0.0, 1.0));

	outColour = vec4(colour, 1.0);
}
