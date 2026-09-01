#version 450

layout(location = 0) in vec3 inNormalWS;
layout(location = 0) out vec4 outNormalWS;

void main()
{
    outNormalWS = vec4(normalize(inNormalWS), 1.0);
}
