#version 460

#include "include/material.glsl"
#include "include/PointShadowMap.glsl"
#include "include/Light.glsl"

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec2 inUV;

//layout(location = 0) out float o_Depth;

layout(set = 0, binding = 0) uniform SceneData {
  mat4 view;
  mat4 proj;
  mat4 viewproj;
  vec3 cameraPosition;

  float nearPlane;
  float farPlane;
} sceneData;

void main()
{
  const float alpha = texture(textureSamplers[0], inUV).a;
  if (alpha < 0.1) {
    discard;
  }

  const PointLight light = lightData.PointLights[PointLightConstants.LightIndex];
  const float distanceToLight = length(inPos.xyz - light.Position);
  gl_FragDepth = distanceToLight / light.Radius;

}
