#extension GL_EXT_buffer_reference2 : require

struct MaterialData 
{
	vec4 colorFactors;
	float metallicFactor;
	float roughnessFactor;

	float pad0;
	float pad1;

	uint HasNormalMap;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer{ 
	MaterialData materials;
};

layout(set = 1, binding = 0) uniform sampler2D textureSamplers[];
//0 - color
//1 - metalroughness
//2 - normal
//3 - emmisive
//4 - occlusion