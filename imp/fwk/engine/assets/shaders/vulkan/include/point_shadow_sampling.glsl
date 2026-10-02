float linearisePointShadowDepth(float depth, float nearPlane, float farPlane)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

float samplePointShadowDistance(samplerCube shadowCube, vec3 sampleDir, float nearPlane, float farPlane)
{
	vec3 a = abs(sampleDir);
	float majorAxis = max(max(a.x, a.y), a.z);
	float axisDistance = linearisePointShadowDepth(texture(shadowCube, sampleDir).r, nearPlane, farPlane);

	return axisDistance * length(sampleDir) / majorAxis;
}

float computePointShadowFactor(samplerCube shadowCube, vec3 lightPos, float nearPlane, float farPlane,
	float depthBias, float normalOffsetTexels, float filterRadiusTexels, float resolution,
	vec3 worldPos, vec3 normal, vec2 screenPos)
{
	float dist = distance(worldPos, lightPos);
	if (dist >= farPlane)
		return 1.0;

	float texelWorld = 2.0 * dist / resolution;
	vec3 toReceiver = (worldPos + normal * (texelWorld * normalOffsetTexels)) - lightPos;
	float receiverDist = length(toReceiver);
	if (receiverDist < 1e-4)
		return 1.0;

	vec3 dir = toReceiver / receiverDist;
	vec3 helper = abs(dir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(helper, dir));
	vec3 bitangent = cross(dir, tangent);

	float ditherAngle = interleavedGradientNoise(screenPos) * 6.28318530718;
	float radius = texelWorld * filterRadiusTexels;

	float lit = 0.0;
	for (int i = 0; i < 8; ++i)
    {
        vec2 offset = rotateDisk(kPoissonDisk[i], ditherAngle) * radius;
        vec3 sampleDir = toReceiver + tangent * offset.x + bitangent * offset.y;
        float occluderDist = samplePointShadowDistance(shadowCube, sampleDir, nearPlane, farPlane);
        lit += (receiverDist - depthBias > occluderDist) ? 0.0 : 1.0;
    }
    lit /= 8.0;

	float fade = smoothstep(0.85 * farPlane, farPlane, dist);
	return mix(lit, 1.0, fade);
}
