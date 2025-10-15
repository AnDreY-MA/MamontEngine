#extension GL_ARB_gpu_shader_int64 : enable

layout(set = 0, binding = 0) uniform SceneData{   
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec3 lightdirection;
} sceneData;

layout(set = 0, binding = 1) uniform sampler2DArray shadowMap;

layout(set = 1, binding = 0) uniform GLTFMaterialData{   
	vec4 colorFactors;
	vec4 metal_rough_factors;
	int colorTexID;
	int metalRoughTexID;
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D colorMap;

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
	uint64_t objectID;
} PushConstants;
