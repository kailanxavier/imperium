#version 450
#extension GL_KHR_vulkan_glsl: enable

struct ProbeState
{
	vec4 offsetAndState;
};

layout(std430, binding = 0) readonly buffer ProbeStates
{
	ProbeState states[];
};

layout(push_constant) uniform PushConstants
{
	mat4 viewProj;
	vec4 cameraForwardAndRadius;
	vec4 minCornerAndSpacing;
	uint probeCountX;
	uint probeCountY;
	uint probeCountZ;
	uint showInactive;
	uint activeWindowOffset;
	uint activeWindowCount;
} pc;

layout(location = 0) out vec2 outLocalUV;
layout(location = 1) out flat uint outProbeIndex;
layout(location = 2) out flat float outActive;
layout(location = 3) out flat float outInActiveWindow;

const vec2 kQuadCorners[6] = vec2[6](
	vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
	vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
);

void main()
{
	uint probeIndex = uint(gl_InstanceIndex);
	uint x = probeIndex % pc.probeCountX;
	uint y = (probeIndex / pc.probeCountX) % pc.probeCountY;
	uint z = probeIndex / (pc.probeCountX * pc.probeCountY);

	ProbeState state = states[probeIndex];
	float isActive = state.offsetAndState.w;

	uint totalProbes = pc.probeCountX * pc.probeCountY * pc.probeCountZ;
	uint relativeIndex = (probeIndex + totalProbes - pc.activeWindowOffset) % totalProbes;
	float inActiveWindow = (relativeIndex < pc.activeWindowCount) ? 1.0 : 0.0;

	vec3 gridPos = pc.minCornerAndSpacing.xyz + vec3(x, y, z) * pc.minCornerAndSpacing.w;
	vec3 probePosWS = gridPos + state.offsetAndState.xyz;

	vec3 forward = normalize(pc.cameraForwardAndRadius.xyz);
	vec3 worldUp = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(worldUp, forward));
	vec3 up = cross(forward, right);

	float radius = pc.cameraForwardAndRadius.w;
	if (isActive < 0.5)
		radius *= (pc.showInactive != 0u) ? 0.5 : 0.0;

	vec2 corner = kQuadCorners[gl_VertexIndex % 6];
	vec3 worldPos = probePosWS + (right * corner.x + up * corner.y) * radius;

	gl_Position = pc.viewProj * vec4(worldPos, 1.0);
	outLocalUV = corner;
	outProbeIndex = probeIndex;
	outActive = isActive;
	outInActiveWindow = inActiveWindow;
}
