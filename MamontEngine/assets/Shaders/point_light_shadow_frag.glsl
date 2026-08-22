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

  PointLight light = lightData.PointLights[PointLightConstants.LightIndex];
  gl_FragDepth = length(inPos.xyz - light.Position) / light.Radius;
}
