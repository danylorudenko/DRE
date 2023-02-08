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

layout(set = 3, binding = 0, std140) uniform TransformUniform
{
	mat4  mvp_mat;
	mat4  model_mat;
	uvec2 textureIDs;
} transformUniform;


void main()
{
	vec4 diffuse = texture(sampler2D(GetGlobalTexture(transformUniform.textureIDs[0]), GetSamplerLinear()), in_uv);
	finalColor = vec4(diffuse.rgb, in_tangent_light.r);
}
