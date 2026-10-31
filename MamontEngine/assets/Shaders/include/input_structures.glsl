#extension GL_ARB_gpu_shader_int64 : require

#include "vertex_data.glsl"
#include "material.glsl"
#include "Light.glsl"
#include "scene_data.glsl"


layout(push_constant) uniform constants
{
  mat4 render_matrix;
  SceneDataBuffer sceneData;
  VertexBuffer vertexBuffer;
  MaterialBuffer materialBuffer;
  uint64_t objectID;
  uint cascadeIndex;
  uint materialIndex;
} PushConstants;
