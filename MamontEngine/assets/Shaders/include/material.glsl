#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_nonuniform_qualifier : require

struct MaterialData 
{
	vec4 colorFactors;
	float metallicFactor;
	float roughnessFactor;
	uint flags;

	float pad0;
	float pad1;

	uint BaseColorTextureIndex;
	uint MetalRoughTextureIndex;
	uint HasNormalMap;
	uint HasEmmisive;
	uint HasOcclusion;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer{ 
	MaterialData materials;
};

layout(set = 1, binding = 10) uniform sampler2D bindlessTextures[];
//0 - color
//1 - metalroughness
//2 - normal
//3 - emmisive
//4 - occlusion

