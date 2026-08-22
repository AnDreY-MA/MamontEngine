#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;
layout(location = 4) out vec4 outTangent;
layout(location = 5) out vec3 outViewPos;

void main()
{
  const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

  const vec4 worldPosition = PushConstants.render_matrix * vec4(v.position, 1.0);

  outNormal = mat3(PushConstants.render_matrix) * v.normal;
  //outNormal = v.normal;
  outColor = v.color;
  outUV = v.uv;
  outTangent = v.tangent;
  //outPos = (PushConstants.render_matrix * position).xyz;
  outPos = worldPosition.xyz;
  outViewPos = (sceneData.view * worldPosition).xyz;

  gl_Position = sceneData.viewproj * worldPosition;

}
