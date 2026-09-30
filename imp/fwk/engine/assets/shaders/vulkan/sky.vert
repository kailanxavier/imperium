#version 450

layout(location = 0) out vec3 outViewDirWS;

layout(push_constant) uniform PushConstants
{
	mat4 invViewProj;
	vec4 cameraPositionWS;
} pc;

void main()
{
	vec2 ndc;
	ndc.x = float((gl_VertexIndex << 1) & 2) * 2.0 - 1.0;
	ndc.y = float(gl_VertexIndex & 2) * 2.0 - 1.0;

	vec4 clipFar = vec4(ndc, 1.0, 1.0);
	vec4 worldFar = pc.invViewProj * clipFar;
	worldFar /= worldFar.w;

	outViewDirWS = worldFar.xyz - pc.cameraPositionWS.xyz;

	gl_Position = vec4(ndc, 1.0, 1.0);
}
