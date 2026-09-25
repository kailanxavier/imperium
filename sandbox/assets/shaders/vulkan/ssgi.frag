#version 450
#include "include/clip_space.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outIndirect;

layout(binding = 0) uniform sampler2D depthTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D historyColourTex;

layout(binding = 3) uniform SSGIParamsUBO
{
    mat4 invProj;
    mat4 invView;
    mat4 view;
    vec4 params; // x = radius, y = intensity, z = sliceCount, w = stepCount
    vec4 params2; // x = thickness, y = maxRadiance, zw = screenSize
} ssgi;

const float PI = 3.14159265359;

vec3 reconstructViewPos(vec2 uv, float depth)
{
    vec4 clip = vec4(uvToNdc(uv), depth, 1.0);
    vec4 view = ssgi.invProj * clip;
    return view.xyz / view.w;
}

float interleavedGradientNoise(vec2 p)
{
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(p, magic.xy)));
}

void main()
{
    float centreDepth = texture(depthTex, inUV).r;
    if (centreDepth >= 1.0)
    {
        outIndirect = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 positionVS = reconstructViewPos(inUV, centreDepth);
    vec3 normalWS = normalize(texture(normalTex, inUV).xyz);
    vec3 normalVS = normalize(mat3(ssgi.view) * normalWS);
    vec3 viewDirVS = normalize(-positionVS);

    float radius = ssgi.params.x;
    float intensity = ssgi.params.y;
    int sliceCount = int(ssgi.params.z);
    int stepCount = int(ssgi.params.w);
    float thickness = ssgi.params2.x;
    float maxRadiance = max(ssgi.params2.y, 0.0);
    vec2 screenSize = ssgi.params2.zw;

    float radiusPx = clamp((radius / max(-positionVS.z, 0.001)) * screenSize.y * 0.5, 1.0, screenSize.y);

    float jitter = interleavedGradientNoise(gl_FragCoord.xy);
    float n = dot(normalVS, viewDirVS);

    vec3 bounce = vec3(0.0);
    float sampleCount = 0.0;

    for (int slice = 0; slice < sliceCount; ++slice)
    {
        float sliceAngle = (float(slice) + jitter) * PI / float(sliceCount);
        vec2 sliceDir = vec2(cos(sliceAngle), sin(sliceAngle));

        for (int side = 0; side < 2; ++side)
        {
            float sign = (side == 0) ? 1.0 : -1.0;

            float maxCosHorizon = -1.0;
            vec2 horizonUV = inUV;
            bool foundHorizon = false;

            for (int step = 1; step <= stepCount; ++step)
            {
                float t = (float(step) + jitter * 0.5) / float(stepCount);
                vec2 offsetPx = sliceDir * sign * t * radiusPx;
                vec2 sampleUV = inUV + offsetPx / screenSize;

                if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
                    continue;

                float sampleDepth = texture(depthTex, sampleUV).r;
                if (sampleDepth >= 1.0)
                    continue;

                vec3 samplePosVS = reconstructViewPos(sampleUV, sampleDepth);
                vec3 horizonVec = samplePosVS - positionVS;
                float horizonLen = length(horizonVec);
                if (horizonLen < 0.0001 || horizonLen > radius * (1.0 + thickness))
                    continue;

                float cosHorizon = dot(horizonVec / horizonLen, viewDirVS);
                float falloff = clamp(1.0 - (horizonLen / radius - 1.0) / max(thickness, 0.001), 0.0, 1.0);
                cosHorizon = mix(-1.0, cosHorizon, falloff);

                if (cosHorizon > maxCosHorizon)
                {
                    maxCosHorizon = cosHorizon;
                    horizonUV = sampleUV;
                    foundHorizon = true;
                }
            }

            float horizonAngle = acos(clamp(maxCosHorizon, -1.0, 1.0));
            float nAngle = acos(clamp(n, -1.0, 1.0));
            float visAngle = clamp(horizonAngle - nAngle, 0.0, PI * 0.5);
            float openness = cos(visAngle) * 0.5 + 0.5;

            if (foundHorizon)
            {
                float horizonDepth = texture(depthTex, horizonUV).r;
                vec3 horizonPosVS = reconstructViewPos(horizonUV, horizonDepth);
                vec3 horizonDirVS = normalize(horizonPosVS - positionVS);

                vec3 horizonNormalWS = normalize(texture(normalTex, horizonUV).xyz);
                vec3 horizonNormalVS = normalize(mat3(ssgi.view) * horizonNormalWS);

                float receiverCos = max(dot(normalVS, horizonDirVS), 0.0);
                float emitterCos = max(dot(horizonNormalVS, -horizonDirVS), 0.0);

                vec3 emittedColour = texture(historyColourTex, horizonUV).rgb;
                bounce += emittedColour * receiverCos * emitterCos * openness;
            }

            sampleCount += 1.0;
        }
    }

    bounce = (bounce / max(sampleCount, 1.0)) * intensity;
    bounce = min(bounce, vec3(maxRadiance)); // clamp fireflies from the single frame old, unfiltered history sample

    outIndirect = vec4(bounce, 1.0);
}
