#version 460

#extension GL_GOOGLE_include_directive : require

#include "include/input_structures.glsl"
#include "include/Shadow.glsl"

layout (location = 0) out vec2 outUV;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

void main() {

	const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	outUV = v.uv;

	const vec4 worldPosition = PushConstants.render_matrix * vec4(v.position, 1.0);
	//gl_Position = ubo.cascadeViewProjMat[PushConstants.cascadeIndex] * position * PushConstants.render_matrix;
	gl_Position = cascadeShadows.cascadeViewProjMat[PushConstants.cascadeIndex] * worldPosition;
}