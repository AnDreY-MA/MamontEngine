#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"
#include "include/pbr.glsl"

#define SHADOW_MAP_CASCADE_COUNT 4
//#define ambient 0.3

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in vec3 inViewPos;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 2) uniform DirectionLightUBO {
  vec3 cascadeSplits;
  mat4 inverseViewMat;
  vec3 lightDirection;
  float _pad;
  vec3 color;
  bool IsActive;
} directionLight;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
  );

layout(set = 0, binding = 3) uniform CVPM {
  mat4 matrices[SHADOW_MAP_CASCADE_COUNT];
} cascadeViewProjMatrices;

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
  float shadow = 1.0;
  float bias = 0.005;
  const float shadowAmbient = 0.3;

  if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
    const float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
    if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
      shadow = shadowAmbient;
    }
  }
  return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
  ivec2 texDim = textureSize(shadowMap, 0).xy;
  float scale = 0.75;
  float dx = scale * 1.0 / float(texDim.x);
  float dy = scale * 1.0 / float(texDim.y);

  float shadowFactor = 0.0;
  int count = 0;
  int range = 1;

  for (int x = -range; x <= range; x++) {
    for (int y = -range; y <= range; y++) {
      shadowFactor += textureProj(sc, vec2(dx * x, dy * y), cascadeIndex);
      count++;
    }
  }
  return shadowFactor / count;
}

uint GetCascadeIndex()
{
  uint cascadeIndex = 0;
  for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
  {
    if (inViewPos.z < directionLight.cascadeSplits[i])
    {
      cascadeIndex = i + 1;
    }
  }

  return cascadeIndex;
}

vec3 CalculalteNormal()
{
  const vec3 tangentNormal = texture(normalMap, inUV).zyx * 2.0 - 1.0;
  const vec3 N = normalize(inNormal);
  const vec3 T = normalize(inTangent.xyz);
  const vec3 B = normalize(cross(N, T));
  const mat3 TBN = mat3(T, B, N);

  return normalize(TBN * tangentNormal);
}

void main()
{
  const vec3 N = inNormal;
  //normalize(inNormal);
  //GetNormal(normalMap, inNormal, inUV, inPos);
  const vec3 L = normalize(-directionLight.lightDirection);
  const vec3 V = normalize(-inViewPos);
  const vec3 H = normalize(V + L);
  const vec3 R = reflect(-V, N);

  const float dotNL = clamp(dot(N, L), 0.001, 1.0);
  const float dotNV = clamp(abs(dot(N, V)), 0.001, 1.0);
  const float dotNH = clamp(dot(N, H), 0.0, 1.0);
  const float dotVH = clamp(dot(V, H), 0.0, 1.0);

  const vec4 baseColorTexture = texture(colorMap, inUV);
  const vec3 albedo = srgbToLinear(baseColorTexture.rgb * materialData.colorFactors.rgb * inColor.rgb);

  const vec4 baseColor = baseColorTexture * materialData.colorFactors * inColor;

  const vec4 metallicRoughness = texture(metalRoughTex, inUV);
  const float metallic = metallicRoughness.b * materialData.metallicFactor;
  const float roughness = clamp(metallicRoughness.g * materialData.roughnessFactor, 0.05, 1.0);
  const float alphaRoughness = roughness * roughness;

  const vec3 F0 = mix(vec3(0.04), albedo, metallic);

  const vec3 specularColor = mix(F0, albedo, metallic);

  const float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
  const float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  const vec3 specularEnviromentR0 = specularColor.rgb;
  const vec3 specularEnviromentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;
  const vec3 diffuseColor = albedo * (vec3(1.0) - F0) * (1.0 - metallic);

  PBRData pbrData;
  pbrData.dotNV = dotNV;
  pbrData.alphaRoughness = alphaRoughness;
  pbrData.albedo = albedo;
  pbrData.reflectance0 = specularEnviromentR0;
  pbrData.reflectance90 = specularEnviromentR90;
  pbrData.diffuseColor = diffuseColor;
  pbrData.specularColor = specularColor;
  pbrData.F0 = F0;

  const uint cascadeIndex = GetCascadeIndex();

  vec4 shadowCoord = (biasMat * cascadeViewProjMatrices.matrices[cascadeIndex]) * PushConstants.render_matrix * vec4(inPos, 1.0);
  shadowCoord = shadowCoord / shadowCoord.w;

  const float shadow = filterPCF(shadowCoord, cascadeIndex);

  vec3 lightColor = vec3(0.0);

  // Sun Light
  if(directionLight.IsActive)
  {
    lightColor += GetLightContribution(pbrData, N, V, -directionLight.lightDirection, directionLight.color) * shadow;
  }

  if(directionLight.IsActive)
  {
      const vec3 ibl = GetIBLContribution(pbrData, N, R, directionLight.color, samplerBRDFLUT, samplerPrefilteredMap, irradianceMap);

      lightColor += ibl;
  }
  //lightColor += prefilteredColor;

  

  const vec3 gamma = vec3(1.0f / GAMMA);
  vec3 finalColor = lightColor;
  finalColor = Tonemap(finalColor);
  //finalColor = pow(finalColor, gamma);

  outFragColor = vec4(finalColor, baseColor.a);
}
