#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;
layout(binding = 1) uniform sampler2D hdrColour;
layout(binding = 2) uniform sampler2D bloomTexture;

layout(push_constant) uniform PushConstants
{
    float exposure;
    float saturation;
    float bloomIntensity;
    uint bloomEnabled;
} pc;

const vec3 kLuminance = vec3(0.2126, 0.7152, 0.0722);

vec3 Saturate(vec3 colour, float saturation)
{
	float grey = dot(colour, kLuminance);
	return mix(vec3(grey), colour, saturation);
}

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
	return (x * (a * x + b)) / (x * (c * x + d) + e);
}

void main()
{
	vec3 hdr = texture(hdrColour, inUV).rgb;
	if (pc.bloomEnabled != 0u)
		hdr += texture(bloomTexture, inUV).rgb * pc.bloomIntensity;

	vec3 exposed = hdr * pow(2.0, pc.exposure);
	vec3 mapped = Saturate(ACESFilm(exposed), pc.saturation);
	mapped = clamp(mapped, 0.0, 1.0);
	outColour = vec4(mapped, 1.0);
}
