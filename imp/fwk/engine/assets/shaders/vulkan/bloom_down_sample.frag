#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;
layout(binding = 1) uniform sampler2D sourceTexture;

layout(push_constant) uniform PushConstants
{
    vec2 texelSize;
    float threshold;
    float softKnee;
    uint applyThreshold;
} pc;

vec3 softThreshold(vec3 colour, float threshold, float knee)
{
    float brightness = max(colour.r, max(colour.g, colour.b));
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / max(4.0 * knee, 0.0001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
    return colour * contribution;
}

void main()
{
    vec3 c0 = texture(sourceTexture, inUV + vec2(-0.5, -0.5) * pc.texelSize).rgb;
    vec3 c1 = texture(sourceTexture, inUV + vec2( 0.5, -0.5) * pc.texelSize).rgb;
    vec3 c2 = texture(sourceTexture, inUV + vec2(-0.5,  0.5) * pc.texelSize).rgb;
    vec3 c3 = texture(sourceTexture, inUV + vec2( 0.5,  0.5) * pc.texelSize).rgb;
    vec3 colour = (c0 + c1 + c2 + c3) * 0.25;

    if (pc.applyThreshold != 0u)
        colour = softThreshold(colour, pc.threshold, max(pc.softKnee, 0.0001));

    outColour = vec4(colour, 1.0);
}
