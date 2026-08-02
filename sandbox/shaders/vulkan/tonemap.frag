#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;
layout(binding = 1) uniform sampler2D hdrColour;
void main()
{
	vec3 hdr = texture(hdrColour, inUV).rgb;
	vec3 mapped = hdr / (hdr + vec3(1.0)); // reinhard
	outColour = vec4(mapped, 1.0);
}
