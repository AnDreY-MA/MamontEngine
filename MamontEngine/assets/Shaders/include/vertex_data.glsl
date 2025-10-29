#extension GL_EXT_buffer_reference : require

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