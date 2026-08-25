#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_viewport_array2 : enable

#include "include/Light.glsl"
#include "include/PointShadowMap.glsl"

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec2 outUV;

void main()
{
  const Vertex v = PointLightConstants.vertexBuffer.vertices[gl_VertexIndex];
  outUV = v.uv;

  const vec4 worldPosition = PointLightConstants.Model * vec4(v.position, 1.0);

  outPos = worldPosition;

  gl_Layer = gl_InstanceIndex;
  gl_Position = PointLightConstants.LightBuffer.viewProj[PointLightConstants.BufferIndex] * worldPosition;
}
