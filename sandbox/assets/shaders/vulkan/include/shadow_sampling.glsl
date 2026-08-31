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

float findBlocker(sampler2DArray shadowMap, int cascadeIndex, float shadowMapSize, vec3 coords, float bias)
{
    float searchRadius = 4.0 / shadowMapSize;
    float blockerSum = 0.0;
    int blockers = 0;

    for (int i = 0; i < 8; ++i)
    {
        vec2 offset = kPoissonDisk[i] * searchRadius;
        float depth = texture(shadowMap, vec3(coords.xy + offset, float(cascadeIndex))).r;
        if (depth < coords.z - bias)
        {
            blockerSum += depth;
            blockers++;
        }
    }

    if (blockers == 0) return -1.0;
    return blockerSum / float(blockers);
}

float filterPCF(sampler2DArray shadowMap, int cascadeIndex, float shadowMapSize, vec3 coords, float radius, float bias)
{
    float shadow = 0.0;
    int samples = 0;

    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        vec2 offset = vec2(x, y) * radius / shadowMapSize;
        float depth = texture(shadowMap, vec3(coords.xy + offset, float(cascadeIndex))).r;
        shadow += coords.z - bias > depth ? 0.0 : 1.0;
        samples++;
    }

    return shadow / float(samples);
}

float shadowPCSS(sampler2DArray shadowMap, int cascadeIndex, float shadowMapSize, vec3 coords, float bias)
{
    if (coords.z > 1.0 || coords.x < 0.0 || coords.x > 1.0 || coords.y < 0.0 || coords.y > 1.0)
        return 1.0;

    float blocker = findBlocker(shadowMap, cascadeIndex, shadowMapSize, coords, bias);
    if (blocker < 0.0) return 1.0;

    float penumbra = (coords.z - blocker) / blocker;
    float filterRadius = clamp(penumbra * 4.0, 1.0, 6.0);

    return filterPCF(shadowMap, cascadeIndex, shadowMapSize, coords, filterRadius, bias);
}

float computeShadowFactor(sampler2DArray shadowMap, float shadowMapSize,
    int cascadeA, vec3 coordsA, int cascadeB, vec3 coordsB, float blendAmount, float bias)
{
    if (coordsA.z > 1.0 || coordsA.x < 0.0 || coordsA.x > 1.0 || coordsA.y < 0.0 || coordsA.y > 1.0)
        return 1.0;

    float blocker = findBlocker(shadowMap, cascadeA, shadowMapSize, coordsA, bias);
    if (blocker < 0.0)
        return 1.0; // fully lit, no blocker found so we skip PCF and the blend entirely

    float penumbra = (coordsA.z - blocker) / blocker;
    float filterRadius = clamp(penumbra * 3.0, 1.0, 6.0);
    float shadowA = filterPCF(shadowMap, cascadeA, shadowMapSize, coordsA, filterRadius, bias);

    if (blendAmount <= 0.0)
        return shadowA;

    float shadowB = filterPCF(shadowMap, cascadeB, shadowMapSize, coordsB, filterRadius, bias);
    return mix(shadowA, shadowB, blendAmount);
}
