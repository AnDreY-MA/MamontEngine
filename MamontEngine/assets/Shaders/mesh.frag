#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "include/input_structures.glsl"

#define SHADOW_MAP_CASCADE_COUNT 4
#define ambient 0.3

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inColor;
layout (location = 4) in vec4 inTangent;
layout (location = 5) in vec3 inViewPos;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 2) uniform UBO {
    vec3 cascadeSplits;
    mat4 inverseViewMat;
    vec3 lightDirection;
    float _pad;
    int colorCascades;
} ubo;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

layout (set = 0, binding = 3) uniform CVPM {
    mat4 matrices[SHADOW_MAP_CASCADE_COUNT];
} cascadeViewProjMatrices;

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambient;
		}
	}
	return shadow;

}

void main() 
{
    const vec4 textureColor = texture(colorMap, inUV);
	const vec4 color = materialData.colorFactors * textureColor;
    /*if (color.a < 0.5) {
        discard;
    }*/

    uint cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
    {
        if(inViewPos.z < ubo.cascadeSplits[i])
        {
            cascadeIndex = i + 1;
        }
    }

    const vec4 shadowCoord = (biasMat * cascadeViewProjMatrices.matrices[cascadeIndex]) * PushConstants.render_matrix * vec4(inPos, 1.0);
    const float shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);

    const vec3 N = normalize(inNormal);
    const vec3 L = normalize(-ubo.lightDirection);
    const vec3 H = normalize(L + inViewPos);
    const float diffuse = max(dot(N, L), ambient);
    const vec3 lightColor = vec3(1.0);

    outFragColor.rgb = max(lightColor * (diffuse * color.rgb), vec3(0.0));
    outFragColor.rgb *= shadow;
    outFragColor.a = color.a;

}
