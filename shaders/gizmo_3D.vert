#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "common/shaders_common.h"
#include "gizmo_3D.h"
#include "common/vertex/dre_vertex_layout.h"

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec3 out_normal;



void main()
{
    out_wpos = vec3(cb.m_Model * vec4(in_pos, 1.0));
    out_normal = in_norm;
    out_color = in_tan;

    vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);

    gl_Position = ndc_pos;
}