#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 3) in vec4 inInstanceModelRow0;
layout(location = 4) in vec4 inInstanceModelRow1;
layout(location = 5) in vec4 inInstanceModelRow2;
layout(location = 6) in vec4 inInstanceModelRow3;

layout(location = 0) out vec3 outNormalWS;
layout(location = 1) out vec3 outPositionWS;
layout(location = 2) out vec2 outUV;

layout(push_constant) uniform PushConstants
{
	mat4 viewProj;
	mat4 nodeWorld;
} pc;

void main()
{
	mat4 instanceModel = mat4(inInstanceModelRow0, inInstanceModelRow1, inInstanceModelRow2, inInstanceModelRow3);
	mat4 world = instanceModel * pc.nodeWorld;

	vec4 worldPos = world * vec4(inPosition, 1.0);
	outPositionWS = worldPos.xyz;
	outNormalWS = normalize(mat3(world) * inNormal);
	outUV = inUV;

	gl_Position = pc.viewProj * worldPos;
}
