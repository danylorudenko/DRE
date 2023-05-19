#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_tan;
layout(location = 3) in vec3 in_btan;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out vec4 out_wpos;


layout(set = 3, binding = 0, std140) uniform InstanceUniform
{
	mat4  mvp_mat;
	mat4  model_mat;
} instanceUniform;

void main()
{	
	vec4 pos = instanceUniform.mvp_mat * vec4(in_pos, 1.0);
	gl_Position = pos;
	out_wpos = instanceUniform.model_mat * vec4(in_pos, 1.0);
	
}