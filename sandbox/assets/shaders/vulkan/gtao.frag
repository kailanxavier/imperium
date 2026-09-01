#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outAO;

layout(binding = 0) uniform sampler2D depthTex;
layout(binding = 1) uniform sampler2D normalTex;

layout(binding = 2) uniform AOParamsUBO
{
    mat4 invProj;
    mat4 invView;
    mat4 view;
    vec4 params;
    vec4 params2;
} ao;

const float PI = 3.14159265359;

vec3 reconstructViewPos(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = ao.invProj * clip;
    return view.xyz / view.w;
}

float sampleViewDepth(vec2 uv)
{
    float d = texture(depthTex, uv).r;
    vec3 vp = reconstructViewPos(uv, d);
    return vp.z;
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
        outAO = vec4(1.0);
            return;
    }

    vec3 positionVS = reconstructViewPos(inUV, centreDepth);
    vec3 normalWS = texture(normalTex, inUV).xyz;
    vec3 normalVS = normalize(mat3(ao.view) * normalWS);
    vec3 viewDirVS = normalize(-positionVS);

    float radius = ao.params.x;
    float intensity = ao.params.y;
    int sliceCount = int(ao.params.z);
    int stepCount = int(ao.params.w);
    float thickness = ao.params2.x;
    float power = ao.params2.y;
    vec2 screenSize = ao.params2.zw;

    float radiusPx = clamp((radius / max(-positionVS.z, 0.001)) * screenSize.y * 0.5, 1.0, screenSize.y);

    float jitter = interleavedGradientNoise(gl_FragCoord.xy);
    float visibility = 0.0;

    for (int slice = 0; slice < sliceCount; ++slice)
    {
        float sliceAngle = (float(slice) + jitter) * PI / float(sliceCount);
        vec2 sliceDir = vec2(cos(sliceAngle), sin(sliceAngle));

        vec3 sliceDirVS = normalize(vec3(sliceDir, 0.0) * mat3(1.0)); // screen space
        float n = dot(normalVS, viewDirVS);

        for (int side = 0; side < 2; ++side)
        {
            float sign = (side == 0) ? 1.0 : -1.0;
            float maxCosHorizon = -1.0;

            for (int step = 1; step <= stepCount; ++step)
            {
                float t = (float(step) + jitter * 0.5) / float(stepCount);
                vec2 offsetPx = sliceDir * sign * t * radiusPx;
                vec2 sampleUV = inUV + offsetPx / screenSize;

                if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
                    continue;

                float sampleDepth = texture(depthTex, sampleUV).r;
                vec3 samplePosVS = reconstructViewPos(sampleUV, sampleDepth);

                vec3 horizonVec = samplePosVS - positionVS;
                float horizonLen = length(horizonVec);
                if (horizonLen < 0.0001)
                    continue;

                float cosHorizon = dot(horizonVec, viewDirVS) / horizonLen;

                float falloff = clamp(1.0 - (horizonLen / radius - 1.0) / max(thickness, 0.001), 0.0, 1.0);
                cosHorizon = mix(-1.0, cosHorizon, falloff);

                maxCosHorizon = max(maxCosHorizon, cosHorizon);
            }

            float horizonAngle = acos(clamp(maxCosHorizon, -1.0, 1.0));
            float nAngle = acos(clamp(n, -1.0, 1.0));
            float visAngle = clamp(horizonAngle - nAngle, 0.0, PI * 0.5);
            visibility += cos(visAngle) * 0.5 + 0.5;
        }
    }

    visibility /= float(sliceCount * 2);
    visibility = clamp(pow(visibility, power) * intensity + (1.0 - intensity), 0.0, 1.0);

    outAO = vec4(visibility, visibility, visibility, 1.0);
}