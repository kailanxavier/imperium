#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outNormalWS;
layout(location = 1) out vec3 outPositionWS;
layout(location = 2) out vec2 outUV;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    mat4 model;
} pc;


void main()
{
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    outPositionWS = worldPos.xyz;
    outNormalWS = normalize(mat3(pc.model) * inNormal);
    outUV = inUV;
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
