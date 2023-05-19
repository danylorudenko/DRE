#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "shaders_defines.h"

layout(location = 0) in vec4 in_wpos;
layout(location = 0) out vec4 out_wpos;

void main()
{
    out_wpos = vec4(in_wpos.xyz, 1.0);
}
