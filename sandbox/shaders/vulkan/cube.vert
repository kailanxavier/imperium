#version 450

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
} pc;

layout(binding = 0) uniform UBO
{
	vec4 tint;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragTint;

void main()
{
	gl_Position = pc.mvp * vec4(inPosition, 1.0);
	fragUV = inUV;
	fragTint = ubo.tint;
}
