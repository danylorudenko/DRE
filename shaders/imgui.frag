#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 finalColor;

layout(set = 3, binding = 0) uniform ImGuiData
{
	vec4 pos_screenSize;
	uint textureID;
} imGuiData;


void main()
{
	float textureValue = SampleGlobalTextureLinear(imGuiData.textureID, in_uv).x;
	finalColor = vec4(in_color.rgb * textureValue, textureValue);
}
