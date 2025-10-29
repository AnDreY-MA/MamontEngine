#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = sceneData.viewproj * PushConstants.render_matrix * vec4(v.position, 1.0);
}