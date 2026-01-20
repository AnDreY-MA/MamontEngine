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

  float dotNL;
  float dotNV;
  float dotNH;
  float dotVH;
  float dotLH;
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

vec3 Uncharted2Tonemap(vec3 color)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;
  float W = 11.2;
  return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

vec3 Tonemap(vec3 color)
{
  vec3 uncharted_color = Uncharted2Tonemap(color * EXPOSURE);
  uncharted_color = uncharted_color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));
  return pow(uncharted_color, vec3(1.0f / GAMMA));
}

vec3 Tonemap(vec3 color, float exposure)
{
  return vec3(1.0) - exp(-color * exposure);
}

vec3 srgbToLinear(vec3 srgb)
{
  return pow(srgb, vec3(GAMMA));
}

float D_GGX(float dotNH, float roughness, vec3 N, vec3 H)
{
  const vec3 NxH = cross(N, H);
  const float a = dotNH * roughness;
  const float k = roughness / (dot(NxH, NxH) + a * a);
  const float d = k * k * (1.0 / PI);

  return SaturateMediump(d);
}

float Geometric_SchlickmithGGX(float dotNL, float dotNV, float roughness)
{
  const float r = roughness + 1.0;
  const float k = (r * r) / 8.0;
  const float GL = dotNL / (dotNL * (1.0 - k) + k);
  const float GV = dotNV / (dotNV * (1.0 - k) + k);

  return GL * GV;
}

float GeometricOcclusion(float dotNL, float dotNV, float roughness)
{
  const float roughnessSq = roughness * roughness;
  const float attenuationL = 2.0 * dotNL / (dotNL + sqrt(roughnessSq + (1.0 - roughnessSq) * (dotNL * dotNL)));
  const float attenuationV = 2.0 * dotNV / (dotNV + sqrt(roughnessSq + (1.0 - roughnessSq) * (dotNV * dotNV)));

  return attenuationL * attenuationV;
}

vec3 SpecularReflection(PBRData pbrData)
{
  return pbrData.reflectance0 + (pbrData.reflectance90 - pbrData.reflectance0) * pow(clamp(1.0 - pbrData.dotNV, 0.0, 1.0), 5.0);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
  return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float F90)
{
  return F0 + (F90 - F0) * pow(1.0 - cosTheta, 5.0);
}

float F_Schlick(float cosTheta, float F0, float F90)
{
  return F0 + (F90 - F0) * pow(1.0 - cosTheta, 5.0);
}

float Fd_Burley(PBRData pbrData)
{
  const float f90 = 0.5 + 2.0 * pbrData.roughness * pbrData.dotLH;
  const float lightScatter = F_Schlick(pbrData.dotNL, 1.0, f90);
  const float viewScatter = F_Schlick(pbrData.dotNV, 1.0, f90);

  return lightScatter * viewScatter * (1.0 / PI);
}

float V_SmithGGXCorrelated(float dotNV, float dotNL, float alphaRoughness)
{
  const float GGXV = dotNL * sqrt(dotNV * dotNV * (1.0 - alphaRoughness) + alphaRoughness);
  const float GGXL = dotNV * sqrt(dotNL * dotNL * (1.0 - alphaRoughness) + alphaRoughness);
  return 0.5 / (GGXV + GGXL);
}

vec3 SpecularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, const vec3 albedo)
{
  vec3 color = vec3(0.0);

  const vec3 H = normalize(V + L);
  const float dotNH = clamp(dot(N, H), 0.0, 1.0);
  const float dotNV = clamp(dot(N, V), 0.0, 1.0);
  const float dotNL = clamp(dot(N, L), 0.0, 1.0);

  const vec3 lightColor = vec3(1.0);

  if (dotNL > 0.0)
  {
    const float distribution = D_GGX(dotNH, roughness, N, H);
    const float geometric = Geometric_SchlickmithGGX(dotNL, dotNV, roughness);
    const vec3 F = F_Schlick(dotNV, F0);

    const vec3 spec = distribution * F * geometric / (4.0 * dotNL * dotNV + 0.001);
    const vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    color += (kD * albedo / PI + spec) * dotNL;
  }

  return color;
}

vec3 GetIBLContribution(PBRData pbrData, vec3 n, vec3 r, vec3 skyColor, sampler2D samplerBRDFLUT, samplerCube prefilteredMap, samplerCube irradianceMap)
{
  const vec3 F = F_Schlick(pbrData.dotNV, pbrData.F0);

  const float lod = pbrData.roughness * 9.0;

  const vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrData.dotNV, 1.0 - pbrData.roughness))).rgb;
  const vec3 diffuseLight = texture(irradianceMap, n).rgb;
  const vec3 specularLight = textureLod(prefilteredMap, r, lod).rgb;

  const vec3 diffuse = diffuseLight * pbrData.diffuseColor;
  const vec3 specular = specularLight * (pbrData.specularColor * brdf.x + brdf.y);

  const vec3 kD = (1.0 - F) * (1.0 - pbrData.metallic);

  const vec3 result = (diffuse + specular) * skyColor;

  return result;
}

float MicrofaceDistribution(PBRData pbrData)
{
  const float roughnessSq = pbrData.alphaRoughness * pbrData.alphaRoughness;
  const float f = (pbrData.dotNH * roughnessSq - pbrData.dotNH) * pbrData.dotNH + 1.0;

  return roughnessSq / (PI * f * f);
}

float G_SchlickGGX(float NoV, float k)
{
  return NoV / (NoV * (1.0 - k) + k);
}
float G_Smith(float NoV, float NoL, float roughness)
{
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 BRDF_Specular(PBRData pbrData, vec3 F0)
{
  float D = D_GGX(pbrData.dotNH, pbrData.roughness, pbrData.N, pbrData.H);
  float G = G_Smith(pbrData.dotNV, pbrData.dotNL, pbrData.roughness);
  vec3 F = F_Schlick(pbrData.dotVH, F0);

  return (D * G * F) / max(4.0 * pbrData.dotNV * pbrData.dotNL, 0.001);
}

vec3 GetNormal(sampler2D normalMap, vec3 normal, vec2 uv, vec3 position)
{
  const vec4 texel = texture(normalMap, uv);
  vec3 tangentNormal = texel.xyz;
  if (texel.w > 0.999)
  {
    tangentNormal = tangentNormal * 2.0 - 1.0;
  }
  else {
    tangentNormal.xy = texel.xw * 2.0 - 1.0;
    tangentNormal.z = sqrt(1 - dot(tangentNormal.xy, tangentNormal.xy));
  }

  const vec3 q1 = dFdx(position);
  const vec3 q2 = dFdy(position);
  const vec2 st1 = dFdx(uv);
  const vec2 st2 = dFdy(uv);
  const vec3 N = normalize(normal);
  const vec3 T = normalize(q1 * st2.t - q2 * st1.t);
  const vec3 B = -normalize(cross(N, T));

  const mat3 TBN = mat3(T, B, N);

  return normalize(TBN * tangentNormal);
}

vec3 GetLightContribution(PBRData pbrData, vec3 n, vec3 v, vec3 l, vec3 color)
{
  const vec3 h = normalize(l + v);
  pbrData.dotNL = clamp(dot(n, l), 0.001, 1.0);
  pbrData.dotNH = clamp(dot(n, h), 0.0, 1.0);
  pbrData.dotVH = clamp(dot(v, h), 0.0, 1.0);
  pbrData.dotLH = clamp(dot(l, h), 0.0, 1.0);

  const vec3 F = SpecularReflection(pbrData);

  // 1 F_Schlick(pbrData.dotNH, pbrData.F0);
  //0 SpecularReflection(pbrData);
  const float G = GeometricOcclusion(pbrData.dotNL, pbrData.dotNV, pbrData.alphaRoughness);
  const float D = MicrofaceDistribution(pbrData);

  // 1 D_GGX(pbrData.dotNV, pbrData.roughness, n, h);
  // 0 MicrofaceDistribution(pbrData);
  const float V = V_SmithGGXCorrelated(pbrData.dotNV, pbrData.dotNL, pbrData.roughness);
  const float Vc = V_Kelemen(pbrData.dotLH);

  const vec3 fr = (D * V) * F;
  const vec3 Fd = pbrData.diffuseColor * Fd_Lambert();

  const vec3 diffuseContrib = (1.0 - F) * (pbrData.diffuseColor / PI);
  const vec3 specularContrib = F * G * D / (4.0 * pbrData.dotNL * pbrData.dotNV);

  const vec3 resultColor = pbrData.dotNL * color * (diffuseContrib + specularContrib);

  return resultColor;
}
