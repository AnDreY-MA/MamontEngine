#version 450

#include "include/input_structures.glsl"

layout(location = 0) out vec3 outUVW;

void main()
{
    const Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    
    vec3 position = v.position;
    
    const vec4 worldPos = PushConstants.render_matrix * vec4(position , 1.0);
    
    const mat4 view = mat4(mat3(sceneData.view));

    gl_Position = sceneData.proj * view * worldPos;
    
    gl_Position.z = gl_Position.w;
    
    outUVW = position;
    outUVW.xy *= -1.0;
}