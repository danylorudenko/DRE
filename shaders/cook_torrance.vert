#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_tan;
layout(location = 3) in vec3 in_btan;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec2 out_velocity;
layout(location = 3) out mat3 out_TBN;



layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
    mat4  mvp_mat;
    mat4  prev_mvp_mat;
    uvec4 textureIDs;
} instanceUniform;

void main()
{
    out_wpos = vec3(instanceUniform.model_mat * vec4(in_pos, 1.0));
    out_uv = in_uv;

	vec4 ndc_pos = instanceUniform.mvp_mat * vec4(in_pos, 1.0);
    gl_Position = ndc_pos;
	
	vec4 prev_ndc_pos = instanceUniform.prev_mvp_mat * vec4(in_pos, 1.0);
	
    vec2 ndc_velocity = ndc_pos.xy - prev_ndc_pos.xy;
	out_velocity = ndc_velocity * 0.5;
    
    vec3 T = normalize(vec3(instanceUniform.model_mat * vec4(in_tan, 0.0)));
    vec3 B = normalize(vec3(instanceUniform.model_mat * vec4(in_btan, 0.0)));
    vec3 N = normalize(vec3(instanceUniform.model_mat * vec4(in_norm, 0.0)));
    
    out_TBN = mat3(T, B, N);
}