#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inInstanceRow0;
layout(location = 4) in vec4 inInstanceRow1;
layout(location = 5) in vec4 inInstanceRow2;
layout(location = 6) in vec4 inInstanceRow3;

layout(push_constant) uniform PushConstants
{
    mat4 viewProj;
    mat4 nodeWorld;
} pc;

void main()
{
    mat4 instanceMatrix = mat4(inInstanceRow0, inInstanceRow1, inInstanceRow2, inInstanceRow3);
    vec4 worldPos = pc.nodeWorld * instanceMatrix * vec4(inPosition, 1.0);
    vec4 clipPos = pc.viewProj * worldPos;
    clipPos.y = -clipPos.y;
    gl_Position = clipPos;
}
