layout(set = 1, binding = 0) uniform GLTFMaterialData{   
	vec4 colorFactors;
	vec4 metal_rough_factors;
	int colorTexID;
	int metalRoughTexID;
} materialData;

layout(set = 1, binding = 1) uniform sampler2DArray shadowMap;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D colorMap;