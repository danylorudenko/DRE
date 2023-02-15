#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "shaders_common.h"

layout(location = 0) in vec3 in_wpos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in mat3 in_TBN;

layout(location = 0) out vec4 finalColor;

layout(set = 3, binding = 0, std140) uniform InstanceUniform
{
    mat4  model_mat;
    mat4  mvp_mat;
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


#define DiffuseTextureID     instanceUniform.textureIDs[0]
#define NormalTextureID      instanceUniform.textureIDs[1]
#define MetalnessTextureID   instanceUniform.textureIDs[2]
#define RoughnessTextureID   instanceUniform.textureIDs[3]

const vec3 C_LIGHT_DIR = vec3(1.0, 1.0, 1.0);

void main()
{
    vec3 diffuse    = SampleGlobalTextureAnisotropic(DiffuseTextureID, in_uv).rgb;
    vec3 normal     = SampleGlobalTextureAnisotropic(NormalTextureID, in_uv).rgb;
    float metalness = SampleGlobalTextureAnisotropic(MetalnessTextureID, in_uv).r;
    float roughness = SampleGlobalTextureAnisotropic(RoughnessTextureID, in_uv).r;

    vec3 n = normalize(in_TBN * (normal * 2.0 - 1.0));
    vec3 v = normalize(GetCameraPos() - in_wpos);
    vec3 L = normalize(C_LIGHT_DIR);
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

    vec3 res = (kD * diffuse+ specular) * NdotL + ambient;

    finalColor = vec4(res, 1.0);
}
