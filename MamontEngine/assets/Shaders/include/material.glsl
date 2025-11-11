layout(set = 1, binding = 0) uniform GLTFMaterialData{   
	vec4 colorFactors;
	float metallicFactor;
	float roughnessFactor;

	float padd0;
	float pad1;

	/*int colorTexID;
	int metalRoughTexID;*/
} materialData;

layout(set = 0, binding = 1) uniform sampler2DArray shadowMap;

layout(set = 1, binding = 1) uniform sampler2D colorMap;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;