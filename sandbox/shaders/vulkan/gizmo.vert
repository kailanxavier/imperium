#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColour;

layout(push_constant) uniform PushConstants
{
	mat4 viewProj;
} pc;

layout(location = 0) out vec4 outColour;

void main()
{
	gl_Position = pc.viewProj * vec4(inPosition, 1.0);
	outColour = inColour;
}
