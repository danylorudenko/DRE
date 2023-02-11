#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent_viewpos;
layout(location = 3) in vec3 in_tangent_wpos;
layout(location = 4) in vec2 in_uv;
layout(location = 5) in vec3 in_tangent_light;

layout(location = 0) out vec4 finalColor;

layout(set = 3, binding = 0, std140) uniform InstanceUniform
{
	mat4  model_mat;
	mat4  mvp_mat;
	uvec4 textureIDs;
} instanceUniform;


float GGX_NDF(vec3 n, vec3 h, float a)
{
	float a2 = a * a;
	float NdotH = dot(n, h);
	float nh2 = NdotH * NdotH;

	float denom = nh2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return a2 / denom;
}

float ShlickGGX(float NdotV, float k)
{
	float denom = NdotV * (1.0 - k) + k;

	return NdotV / denom;
}

float SmithGGX(vec3 n, vec3 v, vec3 L, float a)
{
	float a1 = (a + 1);
	float k = (a1 * a1) / 8.0;

	float ggx1 = ShlickGGX(dot(n, v), k);
	float ggx2 = ShlickGGX(dot(n, L), k);

	return ggx1 * ggx2;
}


vec3 FresnelShlick(float NdotH, vec3 color, float metalness)
{
	vec3 F0 = mix(vec3(0.04), color, metalness);
	return F0 + (vec3(1.0) - F0) * pow(1.0 - NdotH, 5.0);
}




void main()
{
	vec3 diffuse = SampleGlobalTextureAnisotropic(transformUniform.textureIDs[0], in_uv).rgb;
	float lit = max(0.0, dot(in_normal, in_tangent_light));
	lit = max(lit, 0.3);
	finalColor = vec4(diffuse.rgb * lit, 1.0);
}
