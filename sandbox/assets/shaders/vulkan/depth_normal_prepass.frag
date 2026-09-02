#version 450

layout(location = 0) in vec3 inNormalWS;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec4 outNormalWS;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform MaterialFactorsUBO
{
    vec4 baseColourFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float alphaMode; }
material;

void main()
{
    if (material.alphaMode > 0.5 && material.alphaMode < 1.5)
    {
        float alpha = texture(diffuseTexture, inUV).a * material.baseColourFactor.a;
        if (alpha < material.alphaCutoff)
        discard;
    }

    outNormalWS = vec4(normalize(inNormalWS), 1.0);
}
