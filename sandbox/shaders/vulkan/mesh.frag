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
layout(binding = 5) uniform MaterialFactorsUBO
{
    vec4 baseColourFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float alphaMode;
} material;

void main()
{
    vec4 albedoSample = texture(diffuseTexture, inUV);
    vec3 albedo = albedoSample.rgb * material.baseColourFactor.rgb;
    float alpha = albedoSample.a * material.baseColourFactor.a;

    if (material.alphaMode > 0.5 && material.alphaMode < 1.5 && alpha < material.alphaCutoff)
        discard;

    vec3 N = normalize(inNormalWS);
    vec3 L = normalize(-light.lightDirectionWS.xyz);
    vec3 V = normalize(light.cameraPositionWS.xyz - inPositionWS);
    vec3 H = normalize(L + V);

    float diffuseTerm = max(dot(N, L), 0.0);
    float specularTerm = pow(max(dot(N, H), 0.0), light.shininess) * step(0.0001, diffuseTerm);
    vec3 ambient = light.ambientColour.rgb * albedo;
    vec3 diffuse = light.lightColour.rgb * albedo * diffuseTerm;
    vec3 specular = light.lightColour.rgb * specularTerm * light.specularStrength;

    float outAlpha = (material.alphaMode > 1.5) ? alpha : 1.0;
    outColour = vec4(ambient + diffuse + specular, outAlpha);
}
