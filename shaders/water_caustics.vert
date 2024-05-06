#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_tan;
layout(location = 3) in vec3 in_btan;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out vec3 out_ray_start;
layout(location = 1) out vec3 out_ray_end;

layout(set = 3, binding = 0, std140) uniform PassUniform
{
	mat4 light_ViewProjM;
	mat4 light_InvViewProjM;
} passUniform;
layout(set = 3, binding = 1) uniform texture2D envMap;

layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
	mat4  prev_model_mat;
	uvec4 textureID;
} instanceUniform;



vec3 GenerateWave(vec3 local_pos, vec2 dir, float t, float scale, float wavelength, float speed)
{
	vec3 offsets = vec3(0,0,0);
	
	float phase = dot(local_pos.xz, dir);
	phase *= (2 * PI) / wavelength;
	
	offsets.x = (scale * sin(t * speed + phase)) * dir.x;
	offsets.y = scale * cos(t * speed + phase);
	offsets.z = (scale * sin(t * speed + phase)) * dir.y;
	
	return offsets;
}

vec3 GenerateComplexWave(vec3 local_pos, vec2 dir, float t, float scale, float wavelength, float speed, int count)
{
	vec3 offsets = vec3(0,0,0);
	
	offsets += GenerateWave(local_pos, dir, t, scale, wavelength, speed);
	
	offsets += GenerateWave(local_pos, normalize(dir + vec2(0.2, 0)), t, scale / 10, wavelength / 5, speed / 2);
	offsets += GenerateWave(local_pos, normalize(dir + vec2(0.1, 0)), t, scale / 8, wavelength / 4, speed / 2.5);
	offsets += GenerateWave(local_pos, vec2(-dir.y, dir.x), t, scale / 3, wavelength, speed * 2);
	
	return offsets;
	
}


void main()
{
	float REF_INDEX = GetGenericScalar();
	
	
	vec2 wave_dir = normalize(vec2(0,1));
	
	vec3 offset_inpos = in_pos;
	offset_inpos.xz += (wave_dir * 0.01);
	
	float t = GetTimeS();
	
	float scale = 3;
	float wavelength = 40;
	float speed = 0.5;
	
	int complexity = 2;
	
	vec3 wave_pos = in_pos + GenerateComplexWave(in_pos, wave_dir, t, scale, wavelength, speed, complexity);
	vec3 offset_wave_pos = offset_inpos + GenerateComplexWave(offset_inpos, wave_dir, t , scale, wavelength, speed, complexity);
	
	vec3 tan = normalize(wave_pos - offset_wave_pos);
	vec3 btan = -(vec3(-wave_dir.y, 0.0, wave_dir.x));
	vec3 norm = (cross(btan, tan));
	
    vec3 wpos = vec3(instanceUniform.model_mat * vec4(wave_pos, 1.0));
	vec4 ndc_pos = passUniform.light_ViewProjM * vec4(wpos, 1.0);
	
    gl_Position = ndc_pos;
	
	vec2 uv = ndc_pos.xy * 0.5 + 0.5;
	vec4 env_map_sample = SampleTexture(envMap, GetSamplerLinear(), uv);
	
	vec3 world_norm = vec3(instanceUniform.model_mat * vec4(norm, 0.0));
	vec3 refracted_light = refract(GetSunLightDir(), world_norm, REF_INDEX);
	vec2 uv_refract_dir = ((passUniform.light_ViewProjM * vec4(refracted_light, 0.0)).xy) / GetViewportSize();
	vec3 ray_pos = env_map_sample.xyz;
	vec2 sample_uv = uv;
	for(int i = 0; i < 3; i++)
	{
		if(ray_pos.y < env_map_sample.y || (length(env_map_sample.xyz) < 0.001))
		{
			break;
		}
		else
		{
			ray_pos = env_map_sample.xyz;
			sample_uv += uv_refract_dir;
			env_map_sample = SampleTexture(envMap, GetSamplerLinear(), sample_uv);
		}
	}
	
	gl_Position = (passUniform.light_ViewProjM * vec4(env_map_sample.xyz, 1.0));
	out_ray_start = wpos;
	out_ray_end = ray_pos;
	
}