#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"
#include "shaders_defines.h"
#include "poisson.h"
#include "lighting_model.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec4 in_prev_wpos;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in mat3 in_TBN;

layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;

layout(set = 3, binding = 0) uniform texture2D shadowMap;
layout(set = 3, binding = 1) uniform texture2D forwardColorMap;
layout(set = 3, binding = 2) uniform texture2D depthMap;
layout(set = 3, binding = 3, std140) uniform PassUniform
{
    mat4  shadow_VP;
    vec4  shadow_size;
} passUniform;

layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
    mat4  prev_model_mat;
	uvec4 textureID;
	
} instanceUniform;

#define NormalTextureID instanceUniform.textureID[0]


vec3 FresnelShlickWater(float NdotH, vec3 F0)
{
    return F0 + (vec3(1.0) - F0) * pow(1.0 - NdotH, 5.0);
}

vec3 WaterSpecular(float NdotH, float NdotV, float NdotL, vec3 F0, float roughness)
{
	float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    vec3 F = FresnelShlickWater(NdotL, F0);

    vec3 numerator = NDF * G * F;
    float denum = 4.0 * NdotV * NdotL + 0.001;

    vec3 specular = numerator / denum;

    return specular;
}

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float RaymarchWaterDepth(in vec3 wpos, float currentDepth)
{
	const float max_depth = 1;
	const float step = 0.1;
	const int step_count = int(max_depth / step);
	const vec3 start = vec3(wpos.x, wpos.y - max_depth, wpos.z);
	
	float depth_estimate = max_depth;
	
	float prev_depth_sample = 0.0;
	vec3 prev_test_wpos = vec3(0,0,0);
	for(int i = step_count - 1; i >= 0; i--)
	{
		vec3 test_wpos = wpos - vec3(0.0, step * i, 0.0);
		vec4 test_viewpos = GetCameraViewProjM() * vec4(test_wpos, 1.0);
		vec3 test_ndc = test_viewpos.xyz / test_viewpos.w;
		vec2 test_uv = test_ndc.xy * 0.5 + 0.5;
		float depth_sample = SampleTexture(depthMap, GetSamplerLinearClamp(), test_uv).r;
		if(depth_sample < currentDepth)
		{
			float delta = (currentDepth - depth_sample); // ndc space
			float mixer = (delta / step) / (currentDepth);
			float last_sample_depth = (wpos.y - test_wpos.y);
			float prev_sample_depth = (wpos.y - prev_test_wpos.y);
			return mix(last_sample_depth, prev_sample_depth, mixer);
			//return mixer;
			//return (wpos.y - seabed_wpos.y) * 0.2;
		}
		
		prev_depth_sample = depth_sample;
		prev_test_wpos = test_wpos;
	}
	
	return max_depth;
}


float LinearizeDepth(float d,float zNear,float zFar)
{
	float x = (zFar-zNear) / zNear;
	return 1.0 / (x * d + 1.0);
}

float SimpleWaterDepth(float currentDepth, float comparedDepth)
{
	return (comparedDepth - currentDepth);
}

vec3 CausticContribution(vec3 wpos, vec3 l, vec3 v, vec3 n)
{
	vec3 result = vec3(0,0,0);
	
	vec3 vFlat = normalize(vec3(v.x, 0, v.z));
	vec3 offset = vFlat * dot(v, vFlat);
	
	result = dot(n, vec3(0, 1, 0)).rrr;
	result = offset;
	
	//return 0;
	return result;
}

void main()
{
	const vec3 diffuse = vec3(0, 57, 74) / 255;
	//const vec3 diffuse = vec3(0, 255, 0) / 255;
	const vec3 F0 = vec3(39, 39, 39) / 255;
	
	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	
	vec3 normalMap0 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, in_wpos.xz / 10 + GetTimeS() / 48).rgb);
	vec3 normalMap1 = sRGB2Linear(SampleGlobalTextureLinear(NormalTextureID, in_wpos.zx / 4 + GetTimeS() / 44).rgb);
	
	vec3 normalMap = (normalMap0 + normalMap1);
	normalMap = normalize(normalMap);
	
	normalMap = (normalMap * 2 - 1);
	vec3 n = normalize(in_TBN * normalMap);
	
	vec3 l = -GetMainLightDir();
	vec3 v = normalize(GetCameraPos() - in_wpos);
	vec3 h = normalize(l + v);
	
	float NdotL = max(0.0, dot(n, l));
	float NdotV = max(0.0, dot(n, v));
	float NdotH = max(0.0, dot(n, h));
	
	vec3 specular = WaterSpecular(NdotH, NdotV, NdotL, F0, 0.05);
	
	float sampledDepthLinear = LinearizeDepth(SampleTexture(depthMap, GetSamplerLinearClamp(), pixel_pos_uv).r, 0.1, 100);
	float currentDepthLinear = LinearizeDepth(gl_FragCoord.z, 0.1, 100);
	float waterEyeDepth = SimpleWaterDepth(currentDepthLinear, sampledDepthLinear);
	
	
	vec2 refracted_sample_pos = pixel_pos_uv;
	refracted_sample_pos += normalMap.xy * 0.1 * clamp(waterEyeDepth * 50, 0.0, 1.5); // normal-based + depth-based
	
	float depthSampleRefractedLinear = LinearizeDepth(SampleTexture(depthMap, GetSamplerLinear(), refracted_sample_pos).r, 0.1, 100);
	if((currentDepthLinear - depthSampleRefractedLinear) > 0 )
	{
		refracted_sample_pos = pixel_pos_uv;
		depthSampleRefractedLinear = sampledDepthLinear;
	}
	
	vec3 worldSampleRefracted = SampleTexture(forwardColorMap, GetSamplerLinear(), refracted_sample_pos).rgb;
	vec3 worldSample = worldSampleRefracted;
	
	float refractedWaterDepth = SimpleWaterDepth(currentDepthLinear, depthSampleRefractedLinear);
	vec3 water_diffuse = mix(worldSample, diffuse, clamp(refractedWaterDepth * 30 + 0.2, 0, 1));
	//vec3 water_diffuse = mix(worldSample, diffuse, 0);
	
	vec3 res = specular + water_diffuse;
	
    finalColor = vec4(res, 1.0);
	
	vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
	prev_ndc /= prev_ndc.w;
	
	vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
	vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
	vec2 vel_uv = vel * 0.5;
	
	velocity = vec2(vel_uv);
	
}
