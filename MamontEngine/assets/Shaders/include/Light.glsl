#define MAX_POINT_LIGHT 8
#define SHADOW_FACE_NUM 6

//#include "pbr.glsl"

struct PointLight
{
  vec3 Position;
  float padding0;

  vec3 Color;
  float padding1;

  float Radius;
  float Attenuation;

  int CastShadow;
};

struct DirectionLigth
{
	vec3 Color;
	vec3 Direction;
	int CastShadow;
};

layout(set = 0, binding = 8) uniform samplerCube pointLightShadowSamplers[MAX_POINT_LIGHT];

layout(set = 0, binding = 2) uniform LightDataUBO {
  DirectionLigth DirectLight;
  
  float _pad;
  bool IsActive;

  PointLight PointLights[MAX_POINT_LIGHT];
  int PointLightNum;
} lightData;

float CalcualteDistanceAttenuation(float dist, float range)
{
  const float d = clamp(1.0 - pow((dist / range), 4.0), 0.0, 1.0);
  return d / (dist * dist);
}

float CalculateAngularAttenuation(vec3 lightDirection, vec3 l, vec2 scaleOffset)
{
  const float cd = dot(lightDirection, l);
  float result = clamp(cd * scaleOffset.x + scaleOffset.y, 0.0, 1.0);
  result *= result;

  return result;
}

float CalculateAttenuation(vec3 pos, vec3 lightPos, float lightRadius)
{
  const float dist = length(lightPos - pos);
  const float atten = CalcualteDistanceAttenuation(dist, lightRadius);
  return atten;
}

vec3 CalculatePointLight(PointLight light, vec3 fragPos, float dotNL, vec3 v, vec3 l, vec3 diffuseColor, float roughness, float metallic, vec3 f0, float occlusion)
{
  const vec3 h = normalize(v + l);

  vec3 result = vec3(1.0);
  
  return result;
}