#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out uvec2 outColor;


void main()
{
    //outObjectID = PushConstants.objectID;
    //uint lowerBits = uint(PushConstants.objectID & 0xFFFFFFFFUL); 

    // extract upper 32 bits
    //uint upperBits = uint(PushConstants.objectID >> 32);         

    // convert to uvec2 by packing the parts as floats
    //outColor = uvec2(uint(lowerBits), uint(upperBits));

    uint lowerBits = uint(PushConstants.objectID & 0xFFFFFFFFUL);
    uint upperBits = uint(PushConstants.objectID >> 32);

    outColor = uvec2(lowerBits, upperBits);
}