#extension GL_ARB_gpu_shader_int64 : enable

#include "vertex_data.glsl"
#include "material.glsl"

layout(set = 0, binding = 0) uniform SceneData {
  mat4 view;
  mat4 proj;
  mat4 viewproj;
  vec3 lightdirection;
  vec3 cameraPosition;
} sceneData;

layout(push_constant) uniform constants
{
  mat4 render_matrix;
  VertexBuffer vertexBuffer;
  uint64_t objectID;
  uint cascadeIndex;
} PushConstants;
