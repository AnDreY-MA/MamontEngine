layout(set = 1, binding = 0) uniform GLTFMaterialData {
  vec4 colorFactors;
  float metallicFactor;
  float roughnessFactor;

  float pad0;
  float pad1;

  uint HasNormalMap;

} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorMap;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout(set = 1, binding = 3) uniform sampler2D normalMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;
layout(set = 1, binding = 5) uniform sampler2D occlusionMap;