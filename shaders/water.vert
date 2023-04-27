#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec4 out_prev_wpos;
layout(location = 2) out mat3 out_TBN;



layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
	mat4  prev_model_mat;
} instanceUniform;

void main()
{
    out_wpos = vec3(instanceUniform.model_mat * vec4(in_pos, 1.0));

	vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);
	ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter
	
    gl_Position = ndc_pos;
	out_prev_wpos = instanceUniform.prev_model_mat * vec4(in_pos, 1.0);
	
    vec3 T = normalize(vec3(instanceUniform.model_mat * vec4(1.0, 0.0, 0.0, 0.0)));
    vec3 B = normalize(vec3(instanceUniform.model_mat * vec4(0.0, 0.0, 1.0, 0.0)));
    vec3 N = normalize(vec3(instanceUniform.model_mat * vec4(0.0, 1.0, 0.0, 0.0)));
    
    out_TBN = mat3(T, B, N);
}