#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;

layout(binding = 0) uniform sampler2D ssgiTex;
layout(binding = 1) uniform sampler2D depthTex;
layout(binding = 2) uniform sampler2D normalTex;

layout(binding = 3) uniform BlurParamsUBO
{
    vec2 texelSize;
    float depthSigma;
    float normalSigma;
} blurParams;

void main()
{
    float centreDepth = texture(depthTex, inUV).r;
    vec3 centreNormal = texture(normalTex, inUV).xyz;

    float totalWeight = 0.0;
    vec3 totalColour = vec3(0.0);

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 offset = vec2(x, y) * blurParams.texelSize;
            vec2 sampleUV = inUV + offset;

            float sampleDepth = texture(depthTex, sampleUV).r;
            vec3 sampleNormal = texture(normalTex, sampleUV).xyz;
            vec3 sampleColour = texture(ssgiTex, sampleUV).rgb;

            float depthDiff = abs(sampleDepth - centreDepth);
            float depthWeight = exp(-depthDiff * depthDiff / (2.0 * blurParams.depthSigma * blurParams.depthSigma));

            float normalWeight = pow(max(dot(sampleNormal, centreNormal), 0.0), blurParams.normalSigma);
            float weight = depthWeight * normalWeight;
            totalColour += sampleColour * weight;
            totalWeight += weight;
        }
    }

    outColour = vec4(totalColour / max(totalWeight, 0.0001), 1.0);
}
