#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_prev_wpos;
layout(location = 3) in mat3 in_TBN;

layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;

layout(set = 3, binding = 0) uniform texture2D shadowMap;
layout(set = 3, binding = 1, std140) uniform PassUniform
{
    mat4  shadow_VP;
    vec4  shadow_size;
} passUniform;

layout(set = 4, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
    mat4  prev_model_mat;
    uvec4 textureIDs;
} instanceUniform;


float GGX_NDF(float NdotH, float a)
{
    float a2 = a * a;
    float nh2 = NdotH * NdotH;

    float denom = nh2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

float ShlickGGX(float NdotV, float k)
{
    float denom = NdotV * (1.0 - k) + k;

    return NdotV / denom;
}

float SmithGGX(float NdotV, float NdotL, float a)
{
    float a1 = (a + 1);
    float k = (a1 * a1) / 8.0;

    float ggx1 = ShlickGGX(NdotV, k);
    float ggx2 = ShlickGGX(NdotL, k);

    return ggx1 * ggx2;
}


vec3 FresnelShlick(float NdotH, vec3 color, float metalness)
{
    vec3 F0 = mix(vec3(0.04), color, metalness);
    return F0 + (vec3(1.0) - F0) * pow(1.0 - NdotH, 5.0);
}

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


#define DiffuseTextureID     instanceUniform.textureIDs[0]
#define NormalTextureID      instanceUniform.textureIDs[1]
#define MetalnessTextureID   instanceUniform.textureIDs[2]
#define RoughnessTextureID   instanceUniform.textureIDs[3]

void main()
{
    vec3 diffuse    = SampleGlobalTextureAnisotropic(DiffuseTextureID, in_uv).rgb;
    vec3 normal     = SampleGlobalTextureAnisotropic(NormalTextureID, in_uv).rgb;
    float metalness = SampleGlobalTextureAnisotropic(MetalnessTextureID, in_uv).r;
    float roughness = SampleGlobalTextureAnisotropic(RoughnessTextureID, in_uv).r;

    vec3 n = normalize(in_TBN * (normal * 2.0 - 1.0));
    vec3 v = normalize(GetCameraPos() - in_wpos);
    vec3 L = -GetMainLightDir();
    vec3 h = normalize(v + L);
    
    float NdotL = max(0.0, dot(n, L));
    float NdotH = max(0.0, dot(n, h));
    float NdotV = max(0.0, dot(n, v));

    float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    vec3 F = FresnelShlick(NdotH, diffuse, metalness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    vec3 numerator = NDF * G * F;
    float denum = 4.0 * NdotV * NdotL + 0.001;

    vec3 specular = numerator / denum;

    vec3 ambient = vec3(0.15, 0.15, 0.15) * diffuse;

    float shadow = CalculateShadow(in_wpos);

    vec3 res = shadow * (kD * diffuse+ specular) * NdotL + ambient;    
	
    finalColor = vec4(res, 1.0);
	
	vec4 prev_ndc = GetPrevCameraViewProjM() * in_prev_wpos;
	prev_ndc /= prev_ndc.w;
	
	vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
	vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;
	
	vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
	vec2 vel_uv = vel * 0.5;
	
	velocity = vec2(vel_uv);
	
}
