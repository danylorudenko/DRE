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
layout(set = 3, binding = 2, std140) uniform PassUniform
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

#define ENABLE_PCF 1
#define ENABLE_PCF_POISSON 1

float CalculateShadow(vec3 wpos)
{
    vec3 lightspaceCoord = (passUniform.shadow_VP * vec4(wpos, 1.0)).xyz;
    vec2 shadowUV = lightspaceCoord.xy * 0.5 + 0.5;
    
#ifdef ENABLE_PCF
    vec2 shadowSize = passUniform.shadow_size.xy;
#ifdef ENABLE_PCF_POISSON
    float result = 0.0;
    float sampleCount = 16;
    for(int i = 0; i < sampleCount; i++)
    {
        float val = texture(sampler2D(shadowMap, GetSamplerLinear()), shadowUV + (poisson16[i] * 1) / shadowSize).r;
        result += val - 0.01 > lightspaceCoord.z ? 0.0 : 1.0;
    }
#else
    const int C_FILTER_SIZE = 2;
    vec2 start = floor(shadowUV * shadowSize);

    float result = 0.0;
    float sampleCount = C_FILTER_SIZE * C_FILTER_SIZE;
    for(int i = 0; i < C_FILTER_SIZE; i++)
    {
        for(int j = 0; j < C_FILTER_SIZE; j++)
        {
            float val = texture(sampler2D(shadowMap, GetSamplerNearest()), (start + vec2(i, j)) / shadowSize).r;
            result += val - 0.01 > lightspaceCoord.z ? 0.0 : 1.0;
            
        }
    }
#endif
    result /= sampleCount;
    
#else
    float shadowValue = texture(sampler2D(shadowMap, GetSamplerNearest()), shadowUV).r;
    float result = shadowValue - 0.01 > lightspaceCoord.z ? 0.0 : 1.0;
#endif

    return result;
}


vec3 WaterBRDF(float NdotH, float NdotV, float NdotL, vec3 diffuse, float roughness, float metalness, out vec3 transmission)
{
	float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    vec3 F = FresnelShlick(NdotH, diffuse, metalness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    vec3 numerator = NDF * G * F;
    float denum = 4.0 * NdotV * NdotL + 0.001;

    vec3 specular = numerator / denum;

	transmission = kD;
    return specular;
}

void main()
{
	const vec3 diffuse = vec3(3, 227, 252) / 255;
	
	vec3 normalMap0 = SampleGlobalTextureLinear(NormalTextureID, in_wpos.xz / 2 + GetTimeS() / 12).rgb;
	vec3 normalMap1 = SampleGlobalTextureLinear(NormalTextureID, in_wpos.zx + GetTimeS() / 11).rgb;
	
	vec3 normalMap = normalize(mix(normalMap0, normalMap1, 0.5));
	
	normalMap = (normalMap * 2 - 1);
	vec3 n = normalize(in_TBN * normalMap);
	
	vec3 l = -GetMainLightDir();
	vec3 v = normalize(GetCameraPos() - in_wpos);
	vec3 h = normalize(l + v);
	
	float NdotL = max(0.0, dot(n, l));
	float NdotV = max(0.0, dot(n, v));
	float NdotH = max(0.0, dot(n, h));
	
	vec3 transmissionCoef = vec3(0,0,0);
	vec3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, diffuse, 0.01, 0.99);

	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	vec3 worldSample = SampleTexture(forwardColorMap, GetSamplerNearest(), pixel_pos_uv).rgb;


	vec3 ambient = vec3(0.15, 0.15, 0.15) * diffuse;
	vec3 res = mix(brdf, worldSample, transmissionCoef) + ambient;
	
    finalColor = vec4(res, 1.0);
    //finalColor = vec4(in_TBN[2], 1.0);
	
	vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
	prev_ndc /= prev_ndc.w;
	
	vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
	vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
	vec2 vel_uv = vel * 0.5;
	
	velocity = vec2(vel_uv);
	
}
