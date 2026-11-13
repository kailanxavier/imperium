float spotConeFactor(vec3 L, vec3 spotAxisWS, vec2 spotCone)
{
	float cosAngle = dot(-L, spotAxisWS);
	float t = clamp((cosAngle - spotCone.x) * spotCone.y, 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}

float spotRangeFactor(float distanceToLight, float range)
{
	if (range <= 0.0)
		return 1.0;

	float r = clamp(distanceToLight / range, 0.0, 1.0);
	float w = 1.0 - r * r * r * r;
	return w * w;
}
