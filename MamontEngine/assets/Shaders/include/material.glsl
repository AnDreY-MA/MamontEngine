layout(set = 1, binding = 0) uniform GLTFMaterialData {
  vec4 colorFactors;
  float metallicFactor;
  float roughnessFactor;

  float pad0;
  float pad1;

  /*int colorTexID;
      	int metalRoughTexID;*/
} materialData;

layout(set = 0, binding = 1) uniform sampler2DArray shadowMap;

layout(set = 1, binding = 1) uniform sampler2D colorMap;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;
layout(set = 1, binding = 5) uniform sampler2D occlusionMap;

layout(set = 0, binding = 4) uniform samplerCube samplerCubeMap;
layout(set = 0, binding = 5) uniform sampler2D samplerBRDFLUT;
layout(set = 0, binding = 6) uniform samplerCube samplerPrefilteredMap;
layout(set = 0, binding = 7) uniform samplerCube irradianceMap;