#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec2 in_velocity;
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
    mat4  mvp_mat;
    mat4  prev_mvp_mat;
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

const vec2 poisson32[32] =
{
    vec2(-0.397889, 0.542226),
    vec2(-0.414755, -0.394183),
    vec2(0.131764, -0.713506),
    vec2(0.551543, 0.554334),
    vec2(0.317522, -0.088899),
    vec2(0.927145, 0.283128),
    vec2(0.141766, 0.672284),
    vec2(-0.626308, 0.079957),
    vec2(-0.379704, -0.823208),
    vec2(-0.165635, 0.116704),
    vec2(0.477730, -0.835368),
    vec2(0.823137, -0.082292),
    vec2(-0.254509, 0.914898),
    vec2(-0.029949, -0.332681),
    vec2(-0.735420, 0.649945),
    vec2(0.269829, 0.337499),
    vec2(0.589355, 0.188804),
    vec2(0.495027, -0.463772),
    vec2(0.430761, 0.880621),
    vec2(-0.740073, -0.226115),
    vec2(-0.843081, 0.319486),
    vec2(-0.118380, 0.503956),
    vec2(-0.103058, -0.967695),
    vec2(-0.989892, 0.031239),
    vec2(-0.650113, -0.657721),
    vec2(-0.395081, -0.071884),
    vec2(-0.409406, 0.272306),
    vec2(0.112218, 0.112523),
    vec2(0.258025, -0.346162),
    vec2(0.105651, 0.945739),
    vec2(-0.164829, -0.660185),
    vec2(0.700367, -0.693439)
};

const vec2 poisson16[16] =
{
    vec2(-0.376812, 0.649265),
    vec2(-0.076855, -0.632508),
    vec2(-0.833781, -0.268513),
    vec2(0.398413, 0.027787),
    vec2(0.360999, 0.766915),
    vec2(0.584715, -0.809986),
    vec2(-0.238882, 0.067867),
    vec2(0.824410, 0.543863),
    vec2(0.883033, -0.143517),
    vec2(-0.581550, -0.809760),
    vec2(-0.682282, 0.223546),
    vec2(0.438031, -0.405749),
    vec2(0.045340, 0.428813),
    vec2(-0.311559, -0.328006),
    vec2(-0.054146, 0.935302),
    vec2(0.723339, 0.196795)
};

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
    
    // debug section
    vec2 screenUV = gl_FragCoord.xy / GetViewportSize();
    float shadowValue = texture(sampler2D(shadowMap, GetSamplerLinear()), screenUV).r;
    
    //finalColor = vec4(shadowValue.rrr, 1.0);
    //finalColor = vec4(shadow.rrr, 1.0);
    finalColor = vec4(res, 1.0);
    velocity = in_velocity;
}
