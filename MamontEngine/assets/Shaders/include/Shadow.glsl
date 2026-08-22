#define SHADOW_MAP_CASCADE_COUNT 4

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

float calculatePointShadow(vec3 pos, vec3 lightPos, float NoL, samplerCube shadowMap, float farPlane)
{
  vec3 fragToLight = pos - lightPos;
  fragToLight.z = -fragToLight.z;
  const float currentDepth = length(fragToLight);
  const float bias = max(0.05 * (1.0 - NoL), 0.05);

  float closestDepth = texture(shadowMap, fragToLight).r;
  closestDepth *= farPlane;

  const float result = currentDepth - bias > closestDepth ? 0.0 : 1.0;
  return result;
}
