#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "poisson.h"
#include "lighting_model.h"

layout(location = 0) in vec3 in_ray_start;
layout(location = 1) in vec3 in_ray_end;

layout(location = 0) out float causticMap;


//layout(set = 3, binding = 0, std140) uniform PassUniform
//{
//	mat4 light_ViewProjM;
//	mat4 light_InvViewProjM;
//} passUniform;

layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
	mat4  prev_model_mat;
	uvec4 textureID;
} instanceUniform;

#define NormalTextureID instanceUniform.textureID[0]

void main()
{
	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	
	//vec3 normalMap0 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, in_ray_start.xz / 10 + GetTimeS() / 48).rgb);
	//vec3 normalMap1 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, in_ray_start.zx / 4 + GetTimeS() / 44).rgb);
	//
	//vec3 normalMap = (normalMap0 + normalMap1);
	//normalMap = normalize(normalMap);
	//
	//normalMap = (normalMap * 2 - 1);
	//vec3 n = normalize(in_TBN * normalMap);
	
	float old_area = length(dFdx(in_ray_start) * dFdy(in_ray_start));
	float new_area = length(dFdx(in_ray_end) * dFdy(in_ray_end));
	
	float ratio = old_area / new_area;
	
    causticMap = ratio * 0.3;
	
}
