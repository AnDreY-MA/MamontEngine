#version 460

#include "include/input_structures.glsl"

layout (location = 0) in vec2 inUV;

void main() 
{	
	const float alpha = texture(colorMap, inUV).a;
	if (alpha < 0.5) {
		discard;
	}
}