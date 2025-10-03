#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

#define SHADOW_MAP_CASCADE_COUNT 4

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color;
	vec4 tangent;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(push_constant) uniform PushConstants {
    mat4 render_matrix;
	VertexBuffer vertexBuffer;
	uint cascadeIndex;
} pushConstants;

layout (set = 0, binding = 3) uniform UBO {
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProjMat;
} ubo;

layout (location = 0) out vec2 outUV;

void main() {

	Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;
	vec4 position = vec4(v.position, 1.0f);
    
	gl_Position = ubo.cascadeViewProjMat[pushConstants.cascadeIndex] * position;

}