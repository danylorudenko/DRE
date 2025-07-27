#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "common/shaders_common.h"
#include "common/lighting/lighting.h"
#include "common/forward.h"

#include "common/vertex/dre_vertex_layout.h"

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec4 out_prev_wpos;
layout(location = 3) out mat3 out_TBN;

void main()
{
    S_INSTANCE_GPURef InstanceRef = GetInstance();

    mat4 model_mat = GetWorldTransform(InstanceRef);
    out_wpos = vec3(model_mat * vec4(in_pos, 1.0));
    out_uv = in_uv;

    vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);
    ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter

    gl_Position = ndc_pos;
    out_prev_wpos = GetInvWorldTransform(InstanceRef) * vec4(in_pos, 1.0);

    vec3 T = normalize(vec3(model_mat * vec4(in_tan, 0.0)));
    vec3 B = normalize(vec3(model_mat * vec4(in_btan, 0.0)));
    vec3 N = normalize(vec3(model_mat * vec4(in_norm, 0.0)));

    out_TBN = mat3(T, B, N);
}