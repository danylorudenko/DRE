#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec4 out_prev_wpos;
layout(location = 2) out vec3 out_normal;



layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
	mat4  prev_model_mat;
} instanceUniform;

vec3 GenerateWave(vec3 local_pos, vec2 dir, float t, float scale, float wavelength, float speed)
{
	vec3 wave_pos = local_pos;
	
	float phase = dot(local_pos.xz, dir);
	phase *= (2 * PI) /wavelength;
	
	wave_pos.x += scale * sin(t * speed + phase) * dir.x;
	wave_pos.y += scale * cos(t * speed + phase);
	wave_pos.z += scale * sin(t * speed + phase) * dir.y;
	
	return wave_pos;
}

void main()
{
	vec2 wave_dir = normalize(vec2(1,1));
	
	vec3 offset_inpos = in_pos;
	offset_inpos.xz += (wave_dir * 0.05);
	
	float t = GetTimeS();
	//float t = 0;
	
	vec3 wave_pos = GenerateWave(in_pos, wave_dir, t, 1, 10, 1);
	vec3 offset_wave_pos = GenerateWave(offset_inpos, wave_dir, t , 1, 10, 1);
	
	vec3 tan = normalize(wave_pos - offset_wave_pos);
	vec3 btan = (vec3(-wave_dir.y, 0.0, wave_dir.x));
	vec3 norm = (cross(btan, tan));
	
    out_wpos = vec3(instanceUniform.model_mat * vec4(wave_pos, 1.0));

	vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);
	ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter
	
    gl_Position = ndc_pos;
	out_prev_wpos = instanceUniform.prev_model_mat * vec4(wave_pos, 1.0);
	
	out_normal = norm;
}