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

layout(set = 3, binding = 0) uniform ImGuiData
{
	vec4 pos_screenSize;
	uint textureID;
} imGuiData;


void main()
{
	finalColor = vec4(in_normal.rgb, in_tangent_light.r);
}
