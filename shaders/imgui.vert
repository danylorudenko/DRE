#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

layout(set = 3, binding = 0) uniform ImGuiData
{
	vec4 pos_screenSize;
	uint textureID;
} imGuiData;

void main()
{
	out_uv = in_uv;
	out_color = in_color;

	vec2 scale = vec2(2.0 / imGuiData.pos_screenSize.z, 2.0 / imGuiData.pos_screenSize.w);
	vec2 translate = vec2(-1.0 - imGuiData.pos_screenSize.x * scale.x, -1.0 - imGuiData.pos_screenSize.y * scale.y);
	
	gl_Position = vec4(in_pos.x * scale.x + translate.x, in_pos.y * scale.y + translate.y, 0.5, 1.0);
}