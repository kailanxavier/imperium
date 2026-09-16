vec3 getShadowCoords(mat4 lightViewProj, vec3 worldPos)
{
    vec4 lightSpace = lightViewProj * vec4(worldPos, 1.0);
    lightSpace.xyz /= lightSpace.w;

    vec3 coords;
    coords.x = lightSpace.x * 0.5 + 0.5;
    coords.y = 0.5 - lightSpace.y * 0.5;
    coords.z = lightSpace.z;
    return coords;
}

const vec2 kPoissonDisk[8] = vec2[](
    vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(-0.5, -0.5), vec2(0.5, -0.5),
    vec2(0.0, 0.75), vec2(0.75, 0.0), vec2(0.0, -0.75), vec2(-0.75, 0.0)
);

float sampleCascadeTexel(sampler2D s0, sampler2D s1, sampler2D s2, sampler2D s3, int cascadeIndex, vec2 uv)
{
    if (cascadeIndex == 0) return texture(s0, uv).r;
    if (cascadeIndex == 1) return texture(s1, uv).r;
    if (cascadeIndex == 2) return texture(s2, uv).r;
    return texture(s3, uv).r;
}

// Heavily based on this: https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
float interleavedGradientNoise(vec2 screenPos)
{
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

vec2 rotateDisk(vec2 v, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

float findBlocker(sampler2D s0, sampler2D s1, sampler2D s2, sampler2D s3, 
    int cascadeIndex, float shadowMapSize, vec3 coords, float bias, float ditherAngle)
{
    float searchRadius = 4.0 / shadowMapSize;
    float blockerSum = 0.0;
    int blockers = 0;

    for (int i = 0; i < 8; ++i)
    {
        vec2 offset = rotateDisk(kPoissonDisk[i], ditherAngle) * searchRadius;
        float depth = sampleCascadeTexel(s0, s1, s2, s3, cascadeIndex, coords.xy + offset);
        if (depth < coords.z - bias)
        {
            blockerSum += depth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return blockerSum / float(blockers);
}

float filterPCF(sampler2D s0, sampler2D s1, sampler2D s2, sampler2D s3, int cascadeIndex, 
    float shadowMapSize, vec3 coords, float radius, float bias, float ditherAngle)
{
    const float kMinFilterRadiusTexels = 1.25;
    radius = max(radius, kMinFilterRadiusTexels);

    float shadow = 0.0;
    int samples = 0;

    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 offset = rotateDisk(vec2(x, y), ditherAngle) * radius / shadowMapSize;
        float depth = sampleCascadeTexel(s0, s1, s2, s3, cascadeIndex, coords.xy + offset);
        shadow += coords.z - bias > depth ? 0.0 : 1.0;
        samples++;
    }

    return shadow / float(samples);
}

float computeShadowFactor(sampler2D s0, sampler2D s1, sampler2D s2, sampler2D s3, vec4 shadowMapSizes,
    vec2 screenPos, int cascadeA, vec3 coordsA, int cascadeB, vec3 coordsB, float blendAmount, float bias)
{
    if (coordsA.z > 1.0 || coordsA.x < 0.0 || coordsA.x > 1.0 || coordsA.y < 0.0 || coordsA.y > 1.0)
        return 1.0;

    float ditherAngle = interleavedGradientNoise(screenPos) * 6.28318530718; // should be clear enough but this is 2pi, not magic

    float sizeA = shadowMapSizes[cascadeA];
    float blocker = findBlocker(s0, s1, s2, s3, cascadeA, sizeA, coordsA, bias, ditherAngle);
    if (blocker < 0.0)
        return 1.0; // fully lit, no blocker found so we skip PCF and the blend entirely

    float penumbra = (coordsA.z - blocker) / blocker;
    float filterRadius = clamp(penumbra * 3.0, 1.0, 6.0);
    float shadowA = filterPCF(s0, s1, s2, s3, cascadeA, sizeA, coordsA, filterRadius, bias, ditherAngle);

    if (blendAmount <= 0.0)
        return shadowA;

    float sizeB = shadowMapSizes[cascadeB];
    float shadowB = filterPCF(s0, s1, s2, s3, cascadeB, sizeB, coordsB, filterRadius, bias, ditherAngle);
    return mix(shadowA, shadowB, blendAmount);
}
