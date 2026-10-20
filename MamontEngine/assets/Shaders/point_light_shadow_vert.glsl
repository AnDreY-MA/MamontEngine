#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_EXT_debug_printf : require
#include "include/Light.glsl"
#include "include/PointShadowMap.glsl"

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec2 outUV;

void main()
{
  const Vertex v = PointLightConstants.vertexBuffer.vertices[gl_VertexIndex];

  const vec4 worldPosition = PointLightConstants.Model * vec4(v.position, 1.0);
  const mat4 viewProj = PointLightConstants.LightBuffer.viewProj[PointLightConstants.BufferIndex];

  outPos = worldPosition;
  outUV = v.uv;

  if(gl_VertexIndex == 0)
  {
	debugPrintfEXT("PointShadow viewProj = %v4f", PointLightConstants.LightBuffer.viewProj[PointLightConstants.BufferIndex]);
  }

  //gl_Layer = gl_InstanceIndex;

  gl_Position = viewProj * worldPosition;

}