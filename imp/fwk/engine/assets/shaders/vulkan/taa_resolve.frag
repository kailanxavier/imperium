#version 450
#include "include/clip_space.glsl"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outResolved;
layout(location = 1) out vec4 outHistory;

layout(binding = 0) uniform sampler2D currentTex;
layout(binding = 1) uniform sampler2D historyTex;
layout(binding = 2) uniform sampler2D velocityTex;
layout(binding = 3) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
    mat4 currToPrevClip;
    vec4 resolutionAndInv;
    float feedbackMin;
    float feedbackMax;
    float varianceGamma;
    uint historyValid;
    float rejectFeedback;
} pc;

const vec3 kLuma = vec3(0.2126, 0.7152, 0.0722);
const float kSkyDepth = 0.99999;

float luma(vec3 c)
{
    return dot(c, kLuma);
}

vec3 toResolveSpace(vec3 c)
{
    return c / (1.0 + luma(c));
}

vec3 fromResolveSpace(vec3 c)
{
    return c / max(1.0 - luma(c), 1e-4);
}

// A single NaN or Infinite can poision the entire history,
// so we need to catch them before they get to the history.
vec3 sanitise(vec3 c)
{
    return (any(isnan(c)) || any(isinf(c))) ? vec3(0.0) : max(c, vec3(0.0));
}

vec3 rgbToYCoCg(vec3 c)
{
    return vec3(
            0.25 * c.r + 0.5 * c.g + 0.25 * c.b,
            0.5  * c.r             - 0.5 *  c.b,
           -0.25 * c.r + 0.5 * c.g - 0.25 * c.b
    );
}

vec3 yCoCgToRgb(vec3 c)
{
    return vec3(
            c.x + c.y - c.z,
            c.x       + c.z,
            c.x - c.y - c.z
    );
}

vec3 sampleHistoryCatmullRom(vec2 uv, vec2 texSize)
{
    vec2 samplePos = uv * texSize;
    vec2 tc1 = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - tc1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 tc0 = (tc1 - 1.0) / texSize;
    vec2 tc3 = (tc1 + 2.0) / texSize;
    vec2 tc12 = (tc1 + w2 / w12) / texSize;

    float wTop = w12.x * w0.y;
    float wLeft = w0.x * w12.y;
    float wCentre = w12.x * w12.y;
    float wRight = w3.x * w12.y;
    float wBottom = w12.x * w3.y;

    vec3 sum =
        toResolveSpace(sanitise(texture(historyTex, vec2(tc12.x, tc0.y)).rgb)) * wTop +
        toResolveSpace(sanitise(texture(historyTex, vec2(tc0.x, tc12.y)).rgb)) * wLeft +
        toResolveSpace(sanitise(texture(historyTex, vec2(tc12.x, tc12.y)).rgb)) * wCentre +
        toResolveSpace(sanitise(texture(historyTex, vec2(tc3.x, tc12.y)).rgb)) * wRight +
        toResolveSpace(sanitise(texture(historyTex, vec2(tc12.x, tc3.y)).rgb)) * wBottom;

    return max(sum / (wTop + wLeft + wCentre + wRight + wBottom), vec3(0.0));
}

vec3 clipToBox(vec3 boxMin, vec3 boxMax, vec3 history)
{
    vec3 centre = 0.5 * (boxMax + boxMin);
    vec3 extents = 0.5 * (boxMax - boxMin) + 1e-5;

    vec3 offset = history - centre;
    vec3 scaled = abs(offset / extents);
    float m = max(scaled.x, max(scaled.y, scaled.z));

    return (m > 1.0) ? (centre + offset / m) : history;
}

void main()
{
    ivec2 texSize = textureSize(currentTex, 0);
    ivec2 pix = ivec2(gl_FragCoord.xy);
    vec2 uv = (vec2(pix) + 0.5) * pc.resolutionAndInv.zw;

    vec3 currentRgb = sanitise(texelFetch(currentTex, pix, 0).rgb);
    if (pc.historyValid == 0u)
    {
        outResolved = vec4(currentRgb, 1.0);
        outHistory = outResolved;
        return;
    }

    vec3 centreToYCoCg = vec3(0.0);
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    vec3 cMin = vec3(1e9);
    vec3 cMax = vec3(1e-9);

    float closestDepth = 2.0;
    ivec2 closestPix = pix;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 p = clamp(pix + ivec2(x, y), ivec2(0), texSize - 1);
            vec3 s = rgbToYCoCg(toResolveSpace(sanitise(texelFetch(currentTex, p, 0).rgb)));
            m1 += s;
            m2 += s * s;
            cMin = min(cMin, s);
            cMax = max(cMax, s);

            if (x == 0 && y == 0)
                centreToYCoCg = s;

            float d = texelFetch(depthTex, p, 0).r;
            if (d < closestDepth)
            {
                closestDepth = d;
                closestPix = p;
            }
        }
    }

    // Where was this frame last pixel?
    vec2 prevUV;
    if (closestDepth >= kSkyDepth)
    {
        vec4 prevClip = pc.currToPrevClip * vec4(uvToNdc(uv), 1.0, 1.0);
        prevUV = ndcToUv(prevClip.xy / prevClip.w);
    }
    else
    {
        vec2 velocity = texelFetch(velocityTex, closestPix, 0).rg;
        prevUV = uv - vec2(velocity.x, -velocity.y);
    }

    // We reprojected outside the screen, no history.
    if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
    {
        outResolved = vec4(currentRgb, 1.0);
        outHistory = outResolved;
        return;
    }

    // Variance clipping taken from Marco Salvi's 2016 GDC talk.
    // The idea is to build the colour box from the neighbour's mean and SD,
    // then intersect them with their respective min/max so extrapolant cases
    // can't stretch it. Words are hard.
    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    vec3 mean = m1 / 9.0;
    vec3 sigma = sqrt(max(m2 / 9.0 - mean * mean, vec3(0.0)));
    vec3 boxMin = max(mean - pc.varianceGamma * sigma, cMin);
    vec3 boxMax = min(mean + pc.varianceGamma * sigma, cMax);

    vec3 historyYCoCg = rgbToYCoCg(sampleHistoryCatmullRom(prevUV, vec2(texSize)));
    vec3 clippedHistory = clipToBox(boxMin, boxMax, historyYCoCg);

    // Trust the history less the further it disagrees with what
    // we see now. The 0.2 floor stops smaller absolute differences in near black regions.
    float lumCurrent = centreToYCoCg.x;
    float lumHistory = clippedHistory.x;
    float unbiasedDiff = abs(lumCurrent - lumHistory) / max(max(lumCurrent, lumHistory), 0.2);
    float agreement = 1.0 - unbiasedDiff;
    float feedback = mix(pc.feedbackMin, pc.feedbackMax, agreement * agreement);

    float excess = max(max(cMin.x - historyYCoCg.x, historyYCoCg.x - cMax.x), 0.0);
    float lightingChange = smoothstep(0.05, 0.25, excess / max(mean.x, 0.2));
    feedback = mix(feedback, min(feedback, pc.rejectFeedback), lightingChange);

    vec3 resolved = fromResolveSpace(yCoCgToRgb(mix(centreToYCoCg, clippedHistory, feedback)));

    // last line of defence, should never really get here.
    resolved = (any(isnan(resolved)) || any(isinf(resolved))) ? currentRgb : max(resolved, vec3(0.0));

    outResolved = vec4(resolved, 1.0);
    outHistory = outResolved;
}
