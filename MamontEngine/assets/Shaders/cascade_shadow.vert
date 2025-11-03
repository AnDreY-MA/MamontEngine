#version 460

#extension GL_GOOGLE_include_directive : require

#include "include/input_structures.glsl"

#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) out vec2 outUV;

layout (set = 0, binding = 3) uniform UBO {
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProjMat;
} ubo;

void main() {

	const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;

	const vec4 position = vec4(v.position, 1.0);
	gl_Position = ubo.cascadeViewProjMat[PushConstants.cascadeIndex] * position * PushConstants.render_matrix;

}