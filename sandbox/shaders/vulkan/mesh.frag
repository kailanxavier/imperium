#version 450
layout(location = 0) in vec3 inNormalWS;
layout(location = 1) in vec3 inPositionWS;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec4 outColour;

const int MAX_LIGHTS = 16;
const float PI = 3.14159265359;

struct GPULight
{
    vec4 positionOrDirWS;
    vec4 colourIntensity;
};

layout(binding = 0) uniform LightUBO
{
    vec4 cameraPositionWS;
    vec4 ambientColour;
    float specularStrength;
    float shininess;
    uint lightCount;
    uint _pad0;
    mat4 sunViewProj;

    vec3 sunDirection;
    float shadowMapSize;

    GPULight lights[MAX_LIGHTS];
} lightData;

layout(binding = 1) uniform sampler2D diffuseTexture;
layout(binding = 2) uniform sampler2D metallicRoughnessTexture;
layout(binding = 3) uniform sampler2D normalTexture;
layout(binding = 4) uniform sampler2D occlusionTexture;

layout(binding = 6) uniform MaterialFactorsUBO
{
    vec4 baseColourFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float alphaMode;
} material;

layout(binding = 5) uniform sampler2D shadowMap;

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // direct lighting remap, standard Karis/Epic convention
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float sampleShadow(vec3 positionWS, vec3 N, vec3 L)
{
    vec4 lightClip = lightData.sunViewProj * vec4(positionWS, 1.0);
    vec3 ndc = lightClip.xyz / lightClip.w;
    vec2 shadowUV = ndc.xy * 0.5 + 0.5;
    float currentDepth = ndc.z;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || currentDepth > 1.0)
        return 1.0; // outside frustum, fully lit

    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0005);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        float sampleDepth = texture(shadowMap, shadowUV + vec2(x, y) * texelSize).r;
        shadow += (currentDepth - bias > sampleDepth) ? 0.0 : 1.0;
    }
    return shadow / 9.0;
}

vec3 getShadowCoords(vec3 worldPos)
{
    vec4 lightSpace = lightData.sunViewProj * vec4(worldPos, 1.0);

    lightSpace.xyz /= lightSpace.w;

    vec3 coords;

    coords.x = lightSpace.x * 0.5 + 0.5;
    coords.y = lightSpace.y * 0.5 + 0.5;
    coords.z = lightSpace.z;

    return coords;
}

float findBlocker(vec3 coords)
{
    float searchRadius = 4.0 / lightData.shadowMapSize;

    float blockerSum = 0.0;
    int blockers = 0;


    for(int x = -3; x <= 3; x++)
    {
        for(int y = -3; y <= 3; y++)
        {
            vec2 offset =
                vec2(x,y) * searchRadius;

            float depth =
                texture(
                    shadowMap,
                    coords.xy + offset
                ).r;

            if(depth < coords.z)
            {
                blockerSum += depth;
                blockers++;
            }
        }
    }


    if(blockers == 0)
        return -1.0;

    return blockerSum / float(blockers);
}

float calculatePenumbra(
    float receiver,
    float blocker)
{
    return
        (receiver - blocker)
        / blocker;
}

float filterPCF(
    vec3 coords,
    float radius)
{
    float shadow = 0.0;

    int samples = 0;


    for(int x=-3;x<=3;x++)
    {
        for(int y=-3;y<=3;y++)
        {
            vec2 offset =
                vec2(x,y)
                * radius
                / lightData.shadowMapSize;

            float depth =
                texture(
                    shadowMap,
                    coords.xy + offset
                ).r;

            shadow +=
                coords.z > depth
                ? 0.0
                : 1.0;

            samples++;
        }
    }

    return shadow / float(samples);
}

float shadowPCSS(vec3 coords)
{
    if(coords.z > 1.0)
        return 1.0;

    float blocker =
        findBlocker(coords);

    // fully lit
    if(blocker < 0.0)
        return 1.0;

    float penumbra =
        calculatePenumbra(
            coords.z,
            blocker
        );

    float filterRadius =
        clamp(
            penumbra * 40.0,
            1.0,
            20.0
        );

    return filterPCF(
        coords,
        filterRadius
    );
}

void main()
{
    vec4 albedoSample = texture(diffuseTexture, inUV);
    vec3 albedo = albedoSample.rgb * material.baseColourFactor.rgb;
    float alpha = albedoSample.a * material.baseColourFactor.a;

    if (material.alphaMode > 0.5 && material.alphaMode < 1.5 && alpha < material.alphaCutoff)
        discard;

    vec3 mrSample = texture(metallicRoughnessTexture, inUV).rgb;
    float roughness = clamp(material.roughnessFactor * mrSample.g, 0.045, 1.0); // floor avoids a2==0 degenerate GGX
    float metallic = clamp(material.metallicFactor * mrSample.b, 0.0, 1.0);
    float occlusion = texture(occlusionTexture, inUV).r;

    vec3 N = normalize(inNormalWS);
    vec3 V = normalize(lightData.cameraPositionWS.xyz - inPositionWS);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 result = lightData.ambientColour.rgb * albedo * occlusion;

    for (uint i = 0u; i < lightData.lightCount; ++i)
    {
        GPULight light = lightData.lights[i];
        bool isPoint = light.positionOrDirWS.w > 0.5;

        vec3 L = isPoint
            ? normalize(light.positionOrDirWS.xyz - inPositionWS)
            : normalize(-light.positionOrDirWS.xyz);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0)
            continue;

        vec3 radiance = light.colourIntensity.rgb * light.colourIntensity.w;

        float D = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 specular = (D * G * F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 diffuse = kD * albedo / PI;

        //float shadowFactor = isPoint ? 1.0 : sampleShadow(inPositionWS, N, L);
        vec3 shadowCoords = getShadowCoords(inPositionWS);
        float shadowFactor = isPoint ? 1.0 : shadowPCSS(shadowCoords);
        result += (diffuse + specular) * radiance * NdotL * shadowFactor;
    }

    float outAlpha = (material.alphaMode > 1.5) ? alpha : 1.0;
    outColour = vec4(result, outAlpha);
}
