#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout: require

#include "vertex_data.glsl"

layout(buffer_reference, std430) readonly buffer PointLightBuffer {
  mat4 viewProj[];
};

layout(push_constant) uniform constants
{
  mat4 Model;
  VertexBuffer vertexBuffer;

  PointLightBuffer LightBuffer;
  int BufferIndex;
  int LightIndex;
} PointLightConstants;
