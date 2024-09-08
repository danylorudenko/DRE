#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "gizmo_3D.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_normal;



void main()
{
    float scale = GetCameraDistance() / (1.0);

    out_wpos = vec3(cb.m_Model * vec4(in_pos * scale, 1.0));
    out_color = in_color;
    out_normal = in_normal;

    vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);

    gl_Position = ndc_pos;
}