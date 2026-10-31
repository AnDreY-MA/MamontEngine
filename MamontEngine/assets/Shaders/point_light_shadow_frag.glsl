#version 460

#include "include/PointShadowMap.glsl"

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec2 inUV;

void main()
{
    /*MaterialBuffer matbuffer = MaterialBuffer(PushConstants.materialBuffer + PushConstants.materialIndex);
    const MaterialData materialData = matbuffer.materials;
  const float alpha = texture(bindlessTextures[nonuniformEXT(materialData.BaseColorTextureIndex)], inUV).a;
  if (alpha < 0.5) {
    discard;
  }

  const PointLight light = lightData.PointLights[PointLightConstants.LightIndex];


  const float distanceToLight = length(inPos.xyz - light.Position);
  gl_FragDepth = distanceToLight / light.Radius;
  */
}