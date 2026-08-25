#version 460

#include "include/material.glsl"
#include "include/PointShadowMap.glsl"
#include "include/Light.glsl"

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec2 inUV;

//layout(location = 0) out float o_Depth;

void main()
{
  const float alpha = texture(colorMap, inUV).a;
  if (alpha < 0.1) {
    discard;
  }

  const PointLight light = lightData.PointLights[PointLightConstants.LightIndex];
  const float distanceToLight = length(inPos.xyz - light.Position);
  gl_FragDepth = distanceToLight / light.Radius;
}
