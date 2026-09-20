#version 450
#include "include/clip_space.glsl"

layout(location = 0) out vec2 outUV;

void main()
{
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(uvToNdc(outUV), 0.0, 1.0);
}
