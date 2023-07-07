#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "shaders_defines.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec3 in_tan;
layout(location = 3) in vec3 in_btan;
layout(location = 4) in vec2 in_uv;

layout(location = 0) out vec3 out_wpos;
layout(location = 1) out vec4 out_prev_wpos;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out mat3 out_TBN;

layout(set = 3, binding = 0) uniform texture2D shadowMap;
layout(set = 3, binding = 1) uniform texture2D forwardColorMap;
layout(set = 3, binding = 2) uniform texture2D depthMap;
layout(set = 3, binding = 3) uniform texture2D heightMap;
layout(set = 3, binding = 4, std140) uniform PassUniform
{
    mat4  shadow_VP;
    vec4  shadow_size;
	vec4  isFFT_XZ_windX;
} passUniform;

vec2 GetWaterVertexDims() { return passUniform.isFFT_XZ_windX.yz; }
bool IsFFT() { return passUniform.isFFT_XZ_windX.x > 0.5; }
float WindXDir() { return passUniform.isFFT_XZ_windX.w; }

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

vec2 CalculateWaterHeightUV(in vec3 local_pos)
{
	return (local_pos.xz + (GetWaterVertexDims() / 2)) / GetWaterVertexDims();
}

void main()
{
	vec2 wave_dir = normalize(vec2(WindXDir(),1));
	
	vec3 offset_inpos = in_pos;
	offset_inpos.xz += (wave_dir * 0.01);
	
	float t = GetTimeS();
	
	float scale = 3;
	float wavelength = 40;
	float speed = 0.5;
	
	int complexity = 2;
	
	vec3 wave_pos;
	vec3 offset_wave_pos;
	
	vec2 custom_uv = CalculateWaterHeightUV(in_pos);
	
	if(IsFFT())
	{
		//float uv_scalar = 1 / (256.0 * 10);
		vec2 offset_uv = CalculateWaterHeightUV(offset_inpos);
		
		float height = SampleTexture(heightMap, GetSamplerLinear(), custom_uv).r;
		float offset_height = SampleTexture(heightMap, GetSamplerLinear(), offset_uv).r;
		
		wave_pos = in_pos;
		wave_pos.y += height;
		
		offset_wave_pos = offset_inpos;
		offset_wave_pos.y += offset_height;
	}
	else
	{
		wave_pos = in_pos + GenerateComplexWave(in_pos, wave_dir, t, scale, wavelength, speed, complexity);
		offset_wave_pos = offset_inpos + GenerateComplexWave(offset_inpos, wave_dir, t , scale, wavelength, speed, complexity);
	}
	
	vec3 tan = normalize(wave_pos - offset_wave_pos);
	vec3 btan = -(vec3(-wave_dir.y, 0.0, wave_dir.x));
	vec3 norm = (cross(btan, tan));
	
    out_wpos = vec3(instanceUniform.model_mat * vec4(wave_pos, 1.0));

	vec4 ndc_pos = GetCameraViewProjM() * vec4(out_wpos, 1.0);
	ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter
	
    gl_Position = ndc_pos;
	out_prev_wpos = instanceUniform.prev_model_mat * vec4(wave_pos, 1.0);
	
	out_uv = custom_uv;
	out_TBN = mat3(tan, btan, norm);
}