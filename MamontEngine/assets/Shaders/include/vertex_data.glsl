#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color;
	vec4 tangent;
}; 

layout(scalar, buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};