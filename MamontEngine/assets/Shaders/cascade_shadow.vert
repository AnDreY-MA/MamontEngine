#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : require

#include "include/input_structures.glsl"
#include "include/Shadow.glsl"

layout (location = 0) out vec2 outUV;


void main() {
    const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;

	const vec4 worldPosition = PushConstants.render_matrix * vec4(v.position, 1.0);
	gl_Position =  lightData.Cascades[PushConstants.cascadeIndex].ProjView * worldPosition;
}