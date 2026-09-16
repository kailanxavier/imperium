#version 450
#extension GL_KHR_vulkan_glsl: enable

struct RayResult
{
	vec4 directionAndDistance;
	vec4 radiance;
};

layout(std430, binding = 0) readonly buffer RayResults
{
	RayResult rays[];
};

struct ProbeState
{
	vec4 offsetAndState;
};

layout(std430, binding = 1) readonly buffer ProbeStates
{
	ProbeState states[];
};

layout(push_constant) uniform PushConstants
{
	mat4 viewProj;
	vec4 minCornerAndSpacing;
	uint probeCountX;
	uint probeCountY;
	uint probeCountZ;
	uint probeIndex;
	uint rayBase;
	float maxRayDistance;
	uint _pad0;
	uint _pad1;
} pc;

layout(location = 0) out vec4 outColour;

void main()
{
	uint rayIndex = uint(gl_VertexIndex) / 2u;
	uint isEnd = uint(gl_VertexIndex) % 2u;

	uint x = pc.probeIndex % pc.probeCountX;
	uint y = (pc.probeIndex / pc.probeCountX) % pc.probeCountY;
	uint z = pc.probeIndex / (pc.probeCountX * pc.probeCountY);

	ProbeState state = states[pc.probeIndex];
	vec3 gridPos = pc.minCornerAndSpacing.xyz + vec3(x, y, z) * pc.minCornerAndSpacing.w;
	vec3 origin = gridPos + state.offsetAndState.xyz;

	RayResult ray = rays[pc.rayBase + rayIndex];
	vec3 dir = ray.directionAndDistance.xyz;
	float dist = ray.directionAndDistance.w;
	bool isMiss = dist >= pc.maxRayDistance - 0.001;
	bool isBackface = ray.radiance.a > 0.5;

	vec3 endPos = origin + dir * dist;
	vec3 worldPos = (isEnd == 0u) ? origin : endPos;

	vec3 colour;
	if (isMiss)
		colour = vec3(0.15, 0.2, 0.3);
	else if (isBackface)
		colour = vec3(1.0, 0.15, 0.05);
	else
		colour = ray.radiance.rgb;

	gl_Position = pc.viewProj * vec4(worldPos, 1.0);
	outColour = vec4(colour, 1.0);
}
