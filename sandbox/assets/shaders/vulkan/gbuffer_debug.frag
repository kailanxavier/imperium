#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;

layout(binding = 0) uniform sampler2D normalMetallicTex;
layout(binding = 1) uniform sampler2D albedoRoughnessTex;
layout(binding = 2) uniform sampler2D velocityTex;

layout(push_constant) uniform PushConstants
{
    uint mode;
} pc;

void main()
{
    vec4 normalMetallic = texture(normalMetallicTex, inUV);
    vec4 albedoRoughness = texture(albedoRoughnessTex, inUV);
    vec2 velocity = texture(velocityTex, inUV).rg;

    if (pc.mode == 1u)
        outColour = vec4(normalMetallic.xyz * 0.5 + 0.5, 1.0);
    else if (pc.mode == 2u)
        outColour = vec4(albedoRoughness.rgb, 1.0);
    else if (pc.mode == 3u)
        outColour = vec4(vec3(albedoRoughness.a), 1.0);
    else if (pc.mode == 4u)
        outColour = vec4(vec3(normalMetallic.a), 1.0);
    else if (pc.mode == 5u)
        outColour = vec4(velocity * 10.0 + 0.5, 0.0, 1.0);
    else
        outColour = vec4(1.0, 0.0, 1.0, 1.0); // we shouldn't get here

}
