#version 450

layout(location = 0) in vec3 inNormalWS;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec4 outNormalMetallic;
layout(location = 1) out vec4 outAlbedoRoughness;

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
    vec4 diffuseSample = texture(diffuseTexture, inUV);

    if (material.alphaMode > 0.5 && material.alphaMode < 1.5)
    {
        float alpha = diffuseSample.a * material.baseColourFactor.a;
        if (alpha < material.alphaCutoff)
        discard;
    }

    vec3 albedo = diffuseSample.rgb * material.baseColourFactor.rgb;

    outNormalMetallic = vec4(normalize(inNormalWS), clamp(material.metallicFactor, 0.0, 1.0));
    outAlbedoRoughness = vec4(albedo, clamp(material.roughnessFactor, 0.045, 1.0));
}
