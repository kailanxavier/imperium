#version 450

layout(location = 0) in vec3 inNormalWS;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inTangentWS;
layout(location = 3) in float inTangentSign;
layout(location = 4) in vec4 inCurrClipPos;
layout(location = 5) in vec4 inPrevClipPos;

layout(location = 0) out vec4 outNormalMetallic;
layout(location = 1) out vec4 outAlbedoRoughness;
layout(location = 2) out vec2 outVelocity;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform MaterialFactorsUBO
{
    vec4 baseColourFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float alphaMode; }
material;

layout(binding = 2) uniform sampler2D normalTexture;

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

    vec3 geometricN = normalize(inNormalWS);
    vec3 T = normalize(inTangentWS);
    vec3 B = normalize(cross(geometricN, T)) * inTangentSign;
    mat3 TBN = mat3(T, B, geometricN);

    vec3 tangentNormal = texture(normalTexture, inUV).xyz;
    tangentNormal = tangentNormal * 2.0 - 1.0;
    vec3 N = normalize(TBN * tangentNormal);

    outNormalMetallic = vec4(N, clamp(material.metallicFactor, 0.0, 1.0));
    outAlbedoRoughness = vec4(albedo, clamp(material.roughnessFactor, 0.045, 1.0));

    vec2 currNDC = inCurrClipPos.xy / inCurrClipPos.w;
    vec2 prevNDC = inPrevClipPos.xy / inPrevClipPos.w;
    outVelocity = (currNDC - prevNDC) * 0.5;
}
