#define SHADOW_MAP_CASCADE_COUNT 3
#define SHADOW_OPACITY 0.5

float textureProj(sampler2DArray shadowMap, vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
  float shadow = 1.0;
  float bias = 0.005;
  const float shadowAmbient = 0.3;

  if (shadowCoord.z > 0.0 && shadowCoord.z < 1.0) {
    const float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
    if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
      shadow = shadowAmbient;
    }
  }
  return shadow;
}

float filterPCF(sampler2DArray shadowMap, vec4 sc, uint cascadeIndex)
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
      shadowFactor += textureProj(shadowMap, sc, vec2(dx * x, dy * y), cascadeIndex);
      count++;
    }
  }
  return shadowFactor / count;
}

uint GetCascadeIndex(vec3 viewPos, vec3 cascadeSplits)
{
  uint cascadeIndex = 0;
  for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
  {
    if (viewPos.z < cascadeSplits[i])
    {
      cascadeIndex = i + 1;
    }
  }

  return cascadeIndex;
}

float GetPointShadowDepth(vec3 fragPos, vec3 lightPos, float lightNearPlane, float lightFarPlane)
{
    const float distanceToLight = length(fragPos - lightPos);

    return (distanceToLight - lightNearPlane) / (lightFarPlane - lightNearPlane);
}

float calculatePointShadow(vec3 fragPos, vec3 lightPos, float dotNL, samplerCube shadowMap, float radius)
{
  const vec3 fragToLight = fragPos - lightPos;

  const float sampledDist = texture(shadowMap, fragToLight).r;
  const float dist = length(fragToLight);

  const float shadow = (dist <= sampledDist + 0.15) ? 1.0 : SHADOW_OPACITY;
  
  return shadow;
}