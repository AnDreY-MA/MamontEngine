#define SHADOW_MAP_CASCADE_COUNT 3
#define SHADOW_OPACITY 0.5

layout (set = 0, binding = 3) uniform CascadeShadowUBO {
	mat4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
    float CascadeSplits[SHADOW_MAP_CASCADE_COUNT];
} cascadeShadows;

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

uint GetCascadeIndex(vec3 viewPos)
{
  uint cascadeIndex = 0;
  for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
  {
    if (viewPos.z < cascadeShadows.CascadeSplits[i])
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
    vec3 fragToLight = fragPos - lightPos;
    fragToLight.y = -fragToLight.y;

    const float currentDepth = length(fragToLight);
    const float bias = max(0.05 * (1.0 - dotNL), 0.05);

    float closestDepth = texture(shadowMap, fragToLight).r;
    closestDepth *= radius;

    const float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;
  
    return shadow;
}

/*const vec3 sample_offset_directions[20] = vec3[](
    vec3( 1, 1, 1), vec3(-1, 1, 1), vec3( 1,-1, 1), vec3(-1,-1, 1),
    vec3( 1, 1,-1), vec3(-1, 1,-1), vec3( 1,-1,-1), vec3(-1,-1,-1),
    vec3( 1, 0, 0), vec3(-1, 0, 0), vec3( 0, 1, 0), vec3( 0,-1, 0),
    vec3( 0, 0, 1), vec3( 0, 0,-1), vec3( 1, 1, 0), vec3(-1, 1, 0),
    vec3( 1,-1, 0), vec3(-1,-1, 0), vec3( 1, 0, 1), vec3(-1, 0, 1)
);


float calculatePointShadow(vec3 fragPos, vec3 lightPos, float dotNL, samplerCube shadowMap, float radius, vec3 normal)
{
  const vec3 fragToLight = fragPos - lightPos;
  const float currentDepth = length(fragToLight);
  const vec3 lightDirection = normalize(lightPos - fragPos);

  float baseBias = 0.015;
  float normalBias = 0.15 * (1.0 - dot(normal, lightDirection));
  float depthWithBias = currentDepth - (baseBias + normalBias);
  float diskRadius = (currentDepth / radius) * 0.05;

  float shadow = 0.0;
  int samples = 20;

    for (int i = 0; i < samples; i++) {
        vec3 sample_vec = fragToLight + diskRadius * sample_offset_directions[i];
        float closest_depth = texture(shadowMap, sample_vec).r * radius;

        shadow += (depthWithBias > closest_depth) ? 1.0 : 0.0;
    }

    return 1.0 - (shadow / samples);
}*/