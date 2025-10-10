#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

#define SHADOW_MAP_CASCADE_COUNT 4

layout (set = 0, binding = 3) uniform UBO {
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProjMat;
} ubo;

layout (location = 0) out vec2 outUV;

void main() {

	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;
	vec4 position = vec4(v.position, 1.0f);
    
	gl_Position = ubo.cascadeViewProjMat[PushConstants.cascadeIndex] * position;

}