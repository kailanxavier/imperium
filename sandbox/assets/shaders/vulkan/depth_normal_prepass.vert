#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 4) in vec4 inInstanceModelRow0;
layout(location = 5) in vec4 inInstanceModelRow1;
layout(location = 6) in vec4 inInstanceModelRow2;
layout(location = 7) in vec4 inInstanceModelRow3;

layout(location = 0) out vec3 outNormalWS;
layout(location = 1) out vec2 outUV;

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

    mat3 normalMatrix = transpose(inverse(mat3(world)));
    outNormalWS = normalize(normalMatrix * inNormal);
    outUV = inUV;

    gl_Position = pc.viewProj * worldPos;
}
