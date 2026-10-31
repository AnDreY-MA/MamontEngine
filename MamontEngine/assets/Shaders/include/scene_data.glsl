layout(buffer_reference, scalar) readonly buffer SceneDataBuffer {
  mat4 view;
  mat4 proj;
  mat4 viewproj;
  vec3 cameraPosition;

  float nearPlane;
  float farPlane;
} sceneDataBuffer;