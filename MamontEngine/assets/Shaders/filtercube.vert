#version 450

#include "include/vertex_data.glsl"

layout(push_constant) uniform PushConstants {
	mat4 mvp;
	VertexBuffer vertexBuffer;
	float roughness;
	uint numSamples;
} pushConsts;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	const Vertex v = pushConsts.vertexBuffer.vertices[gl_VertexIndex];
	outUVW = v.position;
	gl_Position = pushConsts.mvp * vec4(v.position, 1.0);
}
