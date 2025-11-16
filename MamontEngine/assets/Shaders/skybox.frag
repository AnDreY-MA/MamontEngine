#version 450

#include "include/material.glsl"

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outFragColor;

void main()
{
  outFragColor = texture(samplerCubeMap, inUVW);
}
