#version 460

#extension GL_EXT_debug_printf : require

#include "include/input_structures.glsl"

layout(location = 0) in vec2 inUV;

//layout(set = 1, binding = 0) uniform sampler2D textureSamplers[];

void main()
{
    MaterialBuffer matbuffer = MaterialBuffer(PushConstants.materialBuffer + PushConstants.materialIndex);
    const MaterialData materialData = matbuffer.materials;

    const float alpha = texture(bindlessTextures[nonuniformEXT(materialData.BaseColorTextureIndex)], inUV).a;
    if (alpha < 0.5) {
        discard;
    }
}