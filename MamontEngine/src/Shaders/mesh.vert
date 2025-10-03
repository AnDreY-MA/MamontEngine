#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec4 outTangent;

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

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
	uint cascadeIndex;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    
    
    vec4 position = vec4(v.position, 1.0f);
    gl_Position = sceneData.viewproj * PushConstants.render_matrix * position;    

    //outNormal = normalize(mat3(PushConstants.render_matrix) * v.normal);
    outNormal = v.normal;
	outColor = v.color;    
    outUV = v.uv;
	outTangent = v.tangent;
}

