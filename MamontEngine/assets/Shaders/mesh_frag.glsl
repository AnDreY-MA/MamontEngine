#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"
#include "include/pbr.glsl"
#include "include/Shadow.glsl"

//#define ambient 0.6

layout(set = 0, binding = 1) uniform sampler2DArray shadowMap; // DirectionLightShadow
layout(set = 0, binding = 4) uniform samplerCube samplerCubeMap;
layout(set = 0, binding = 5) uniform sampler2D samplerBRDFLUT;
layout(set = 0, binding = 6) uniform samplerCube samplerPrefilteredMap;
layout(set = 0, binding = 7) uniform samplerCube irradianceMap;



layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in vec3 inViewPos;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 3) uniform CVPM {
  mat4 matrices[SHADOW_MAP_CASCADE_COUNT];
} cascadeViewProjMatrices;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
  );

vec3 CalculalteNormal()
{
  if (!bool(materialData.HasNormalMap))
  {
    return normalize(inNormal);
  }
  const vec4 texel = texture(normalMap, inUV);
  vec3 tangent_normal = texel.xyz;
  if (texel.w > 0.999)
  {
    tangent_normal = tangent_normal * 2.0 - 1.0;
  }
  else
  {
    tangent_normal.xy = texel.xw * 2.0 - 1.0;
    tangent_normal.z = sqrt(1 - dot(tangent_normal.xy, tangent_normal.xy));
  }

  vec3 q1 = dFdx(inPos);
  vec3 q2 = dFdx(inPos);
  vec2 st1 = dFdx(inUV);
  vec2 st2 = dFdx(inUV);

  const vec3 N = normalize(inNormal);
  const vec3 T = normalize(q1 * st2.t - q2 * st1.t);
  const vec3 B = normalize(cross(N, T));
  const mat3 TBN = mat3(T, B, N);

  return normalize(TBN * tangent_normal);
  //return vec3(1.f);
}

void main()
{
  const vec3 N = CalculalteNormal();
  //normalize(inNormal);
  //GetNormal(normalMap, inNormal, inUV, inPos);
  const vec3 viewDirection = normalize(sceneData.cameraPosition - inPos);
  const vec3 R = reflect(-viewDirection, N);

  const vec4 baseColorTexture = texture(colorMap, inUV);
  const vec3 albedo = srgbToLinear(baseColorTexture.rgb * materialData.colorFactors.rgb * inColor.rgb);

  const vec4 baseColor = baseColorTexture * materialData.colorFactors * inColor;

  const vec4 metallicRoughness = texture(metalRoughTex, inUV);
  const float metallic = metallicRoughness.b * materialData.metallicFactor;
  const float roughness = clamp(metallicRoughness.g * materialData.roughnessFactor, 0.089, 1.0);
  const float alphaRoughness = roughness * roughness;

  const vec3 F0 = mix(vec3(0.04), albedo, metallic);

  const vec3 specularColor = mix(F0, albedo, metallic);

  const float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
  const float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  const vec3 specularEnviromentR0 = specularColor.rgb;
  const vec3 specularEnviromentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;
  const vec3 diffuseColor = albedo * (vec3(1.0) - F0) * (1.0 - metallic);
  //(1.0 - metallic) * albedo;
  //albedo * (vec3(1.0) - F0) * (1.0 - metallic);

  PBRData pbrData;
  pbrData.N = N;
  
  pbrData.roughness = roughness;
  pbrData.metallic = metallic;
  pbrData.alphaRoughness = alphaRoughness;
  pbrData.albedo = albedo;
  pbrData.reflectance0 = specularEnviromentR0;
  pbrData.reflectance90 = specularEnviromentR90;
  pbrData.diffuseColor = diffuseColor;
  pbrData.specularColor = specularColor;
  pbrData.F0 = F0;

  vec3 lightColor = vec3(0.0);

  // Direction Light
  if (lightData.IsActive)
  {
    const uint cascadeIndex = GetCascadeIndex(inViewPos, lightData.cascadeSplits);

    const vec3 l = normalize(-lightData.lightDirection);

    const vec3 H = normalize(viewDirection + l);
    const float dotNL = clamp(dot(N, l), 0.001, 1.0);

    const vec4 shadowCoord = (biasMat * cascadeViewProjMatrices.matrices[cascadeIndex]) * vec4(inPos, 1.0);

    const float shadow = filterPCF(shadowMap,  shadowCoord / shadowCoord.w, cascadeIndex);
    float atten = 1.0;
    lightColor +=
      (GetLightContribution(pbrData, N, viewDirection, l, H, lightData.color) * lightData.color) * ( atten * dotNL * shadow);
  }

  if (lightData.IsActive)
  {
    const vec3 ibl = GetIBLContribution(pbrData, N, R, lightData.color, samplerBRDFLUT, samplerPrefilteredMap, irradianceMap);

    lightColor += ibl;
  }

  for (int i = 0; i < lightData.PointLightNum; ++i)
  {
    PointLight pointLight = lightData.PointLights[i];
    const float attenuation = pointLight.Attenuation;
    const vec3 c = pointLight.Color;
    const vec3 l = normalize(pointLight.Position - inPos);

    const vec3 H = normalize(viewDirection + l);

    const float dotNL = clamp(dot(N, l), 0.001, 1.0);

    const float shadow = calculatePointShadow(inPos, pointLight.Position, dotNL, pointLightShadowSamplers[i], pointLight.Radius);
    const float atten = CalculateAttenuation(inPos, l, pointLight) * attenuation;

    lightColor += (GetLightContribution(pbrData, N, viewDirection, l, H, c) * c) * (atten * dotNL * shadow);
  }

  //lightColor += prefilteredColor;

  vec3 finalColor = lightColor;
  finalColor = ACESTonemap(finalColor);
  finalColor = gammaCorrect(finalColor, GAMMA);
  //finalColor = pow(finalColor, gamma);

  outFragColor = vec4(finalColor, baseColor.a);
}
