#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;

layout(set = 3, binding = 0, std140) uniform InstanceUniform
{
	mat4  mvp_mat;
} instanceUniform;

void main()
{
	gl_Position = instanceUniform.mvp_mat * vec4(in_pos, 1.0);
}