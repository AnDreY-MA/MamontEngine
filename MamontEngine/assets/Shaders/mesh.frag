#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable

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

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

const SHCoefficients grace = SHCoefficients(
    vec3( 0.3623915,  0.2624130,  0.2326261 ),
    vec3( 0.1759131,  0.1436266,  0.1260569 ),
    vec3(-0.0247311, -0.0101254, -0.0010745 ),
    vec3( 0.0346500,  0.0223184,  0.0101350 ),
    vec3( 0.0198140,  0.0144073,  0.0043987 ),
    vec3(-0.0469596, -0.0254485, -0.0117786 ),
    vec3(-0.0898667, -0.0760911, -0.0740964 ),
    vec3( 0.0050194,  0.0038841,  0.0001374 ),
    vec3(-0.0818750, -0.0321501,  0.0033399 )
);

vec3 calcIrradiance(vec3 nor) {
    const SHCoefficients c = grace;
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    return (
        c1 * c.l22 * (nor.x * nor.x - nor.y * nor.y) +
        c3 * c.l20 * nor.z * nor.z +
        c4 * c.l00 -
        c5 * c.l20 +
        2.0 * c1 * c.l2m2 * nor.x * nor.y +
        2.0 * c1 * c.l21  * nor.x * nor.z +
        2.0 * c1 * c.l2m1 * nor.y * nor.z +
        2.0 * c2 * c.l11  * nor.x +
        2.0 * c2 * c.l1m1 * nor.y +
        2.0 * c2 * c.l10  * nor.z
    );
}

void main() 
{
    const float lightValue = max(dot(inNormal, vec3(0.3f,1.f,0.3f)), 0.1f);

  	const vec3 irradiance = calcIrradiance(inNormal); 

    const vec4 textureColor = texture(colorMap, inUV);
	  //const vec4 color = inColor * textureColor;
    if (textureColor.a < 0.5) {
        discard;
    }

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

    outFragColor.rgb = max(lightColor * (diffuse * textureColor.rgb), vec3(0.0));
    outFragColor.rgb *= shadow;
    outFragColor.a = textureColor.a;

    // Defualt
	//outFragColor = vec4(textureColor.rgb * lightValue + textureColor.rgb * irradiance.x * vec3(0.2f), 1.f);
}
