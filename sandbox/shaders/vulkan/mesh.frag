#version 450

layout(location = 0) in vec3 inNormalWS;
layout(location = 1) in vec3 inPositionWS;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColour;

layout(binding = 0) uniform LightUBO
{
    vec4 lightDirectionWS;
    vec4 lightColour;
    vec4 ambientColour;
    vec4 cameraPositionWS;
    float specularStrength;
    float shininess;
    float _pad0;
    float _pad1;
} light;

layout(binding = 1) uniform sampler2D diffuseTexture;

void main()
{
    vec3 N = normalize(inNormalWS);
    vec3 L = normalize(-light.lightDirectionWS.xyz);
    vec3 V = normalize(light.cameraPositionWS.xyz - inPositionWS);
    vec3 H = normalize(L + V);

    float diffuseTerm = max(dot(N, L), 0.0);
    float specularTerm = pow(max(dot(N, H), 0.0), light.shininess) * step(0.0001, diffuseTerm);

    vec3 albedo = texture(diffuseTexture, inUV).rgb;

    vec3 ambient = light.ambientColour.rgb * albedo;
    vec3 diffuse = light.lightColour.rgb * albedo * diffuseTerm;
    vec3 specular = light.lightColour.rgb * specularTerm * light.specularStrength;

    outColour = vec4(ambient + diffuse + specular, 1.0);
}
