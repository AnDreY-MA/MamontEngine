#define PI 3.1415926535897932384626433832795

const float GAMMA = 2.2;
const float EXPOSURE = 4.5;

#define MEDIUMP_FLT_MAX    65504.0
float SaturateMediump(float value) {
  return min(value, MEDIUMP_FLT_MAX);
}

float Fd_Lambert() {
  return 1.0 / PI;
}

float V_Kelemen(float dotLH)
{
  return 0.25 / (dotLH * dotLH);
}

struct PBRData
{
  vec3 N;
  vec3 H;

  float roughness;
  float alphaRoughness;

  vec3 albedo;
  vec3 reflectance0;
  vec3 reflectance90;
  vec3 diffuseColor;
  vec3 specularColor;
  vec3 F0;

  float metallic;
};

vec3 gammaCorrect(vec3 color, float gamma) {
  return pow(color, vec3(1.0 / gamma));
}

const mat3 ACESInputMat = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
  );

const mat3 ACESOutputMat = mat3(
    1.60475, -0.10208, -0.00327,
    -0.53108, 1.10813, -0.07276,
    -0.07367, -0.00605, 1.07602
  );

vec3 ACESTonemap(vec3 color) {
  vec3 v = ACESInputMat * color;
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return ACESOutputMat * (a / b);
}

vec3 srgbToLinear(vec3 srgb)
{
  return pow(srgb, vec3(GAMMA));
}

float D_GGX(float dotNH, float roughness)
{
  const float a2 = roughness * roughness;
  const float denom = (dotNH * a2 - dotNH) * dotNH + 1.0;

  return a2 / (PI * denom * denom);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
  return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float F90)
{
  return F0 + (F90 - F0) * pow(1.0 - cosTheta, 5.0);
}

float V_SmithGGXCorrelated(float dotNV, float dotNL, float roughness)
{
  const float a2 = roughness * roughness;
  //const float GGXV = dotNL * sqrt(dotNV * dotNV * (1.0 - a2) + a2);
  //const float GGXL = dotNV * sqrt(dotNL * dotNL * (1.0 - a2) + a2);;
  const float GGXL = dotNV * sqrt((-dotNL * a2 + dotNL) * dotNL + a2);
  const float GGXV = dotNL * sqrt((-dotNV * a2 + dotNV) * dotNV + a2);
  return 0.5 / (GGXV + GGXL);
}

vec3 GetIBLContribution(PBRData pbrData, vec3 n, vec3 r, vec3 skyColor, float dotNV, sampler2D samplerBRDFLUT, samplerCube prefilteredMap, samplerCube irradianceMap)
{
  const vec3 F = F_Schlick(dotNV, pbrData.F0);

  const float lod = pbrData.roughness * 6.0;

  const vec3 brdf = (texture(samplerBRDFLUT, vec2(dotNV, 1.0 - pbrData.roughness))).rgb;
  const vec3 diffuseLight = (texture(irradianceMap, n)).rgb;
  const vec3 specularLight = textureLod(prefilteredMap, r, lod).rgb;

  const vec3 diffuse = diffuseLight * pbrData.diffuseColor;
  const vec3 specular = specularLight * (pbrData.specularColor * brdf.x + brdf.y);

  const vec3 result = (diffuse + specular) * skyColor;

  return result;
}

float G_SchlickGGX(float dotNV, float roughness)
{
  const float r = roughness + 1.0;
  const float k = (r * r) / 8.0;
  return dotNV / (dotNV * (1.0 - k) + k);
}
float G_Smith(float NoV, float NoL, float roughness)
{
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 GetLightContribution(PBRData pbrData, vec3 n, vec3 v, vec3 l, vec3 h, vec3 color)
{
  float dotNV = abs(dot(n, v));
  float dotNH = clamp(dot(n, h), 0.0, 1.0);
  float dotHV = clamp(dot(h, v), 0.0, 1.0);
  float dotNL = clamp(dot(n, l), 0.0, 1.0);

  const float D = D_GGX(dotNH, pbrData.roughness);
  const vec3 F = F_Schlick(dotHV, pbrData.F0);
  const float V = V_SmithGGXCorrelated(dotNV, dotNL, pbrData.roughness);

  vec3 Fr = (D * V) * F;
  vec3 Fd = pbrData.diffuseColor * (1.0 / PI);
  const vec3 Frd = Fr + Fd;

  const vec3 result = (color * Frd);
  return Frd;
}