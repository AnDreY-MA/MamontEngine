#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"

layout(location = 0) out vec4 outColor;

void main()
{
  const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

  outColor = v.color;

  vec4 position = vec4(v.position, 1.0);
  gl_Position = sceneData.viewproj * position;
}
