#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;
layout(binding = 1) uniform sampler2D lowerMip;
layout(binding = 2) uniform sampler2D higherMip;

layout(push_constant) uniform PushConstants
{
    vec2 texelSize;
} pc;

void main()
{
    vec3 sum = vec3(0.0);
    sum += texture(lowerMip, inUV + vec2(-1.0, -1.0) * pc.texelSize).rgb;
    sum += texture(lowerMip, inUV + vec2( 0.0, -1.0) * pc.texelSize).rgb * 2.0;
    sum += texture(lowerMip, inUV + vec2( 1.0, -1.0) * pc.texelSize).rgb;
    sum += texture(lowerMip, inUV + vec2(-1.0,  0.0) * pc.texelSize).rgb * 2.0;
    sum += texture(lowerMip, inUV + vec2( 0.0,  0.0) * pc.texelSize).rgb * 4.0;
    sum += texture(lowerMip, inUV + vec2( 1.0,  0.0) * pc.texelSize).rgb * 2.0;
    sum += texture(lowerMip, inUV + vec2(-1.0,  1.0) * pc.texelSize).rgb;
    sum += texture(lowerMip, inUV + vec2( 0.0,  1.0) * pc.texelSize).rgb * 2.0;
    sum += texture(lowerMip, inUV + vec2( 1.0,  1.0) * pc.texelSize).rgb;
    sum /= 16.0;

    vec3 higher = texture(higherMip, inUV).rgb;
    outColour = vec4(higher + sum, 1.0);
}
