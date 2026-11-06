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

layout(set = 0, binding = 0) uniform sampler2DArray shadowMap; // DirectionLightShadow
layout(set = 0, binding = 3) uniform samplerCube samplerCubeMap;
layout(set = 0, binding = 4) uniform sampler2D samplerBRDFLUT;
layout(set = 0, binding = 5) uniform samplerCube samplerPrefilteredMap;
layout(set = 0, binding = 6) uniform samplerCube irradianceMap;
