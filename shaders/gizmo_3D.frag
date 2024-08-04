#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "gizmo_3D.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 finalColor;

void main()
{
    vec3 L = normalize(vec3(1.0, 1.0, 0.0));
    vec3 n = normalize(in_normal);

    float toWhite = dot(n, L) * 0.5 + 0.5;

    vec3 white = vec3(1.0, 1.0, 1.0);
    finalColor = vec4(mix(in_color, white, toWhite), 1.0);
}
