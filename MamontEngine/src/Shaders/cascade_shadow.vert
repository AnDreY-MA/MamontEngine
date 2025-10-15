#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout(location = 0) in vec3 in_pos;

layout(push_constant) uniform PushConstants {
    mat4 vp_matrix;
};

void main() {

	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;
	vec4 position = vec4(v.position, 1.0f);
    
	gl_Position = ubo.cascadeViewProjMat[PushConstants.cascadeIndex] * position;

}