#ifndef _LIGHTING_MODEL_H_
#define _LIGHTING_MODEL_H_

#include "common/shaders_defines.slang"

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


float3 FresnelShlick(float NdotH, float3 color, float metalness)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), color, metalness);
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - NdotH, 5.0f);
}

float3 CookTorranceBRDF(float NdotH, float NdotV, float NdotL, float3 diffuse, float roughness, float metalness)
{
    float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    float3 F = FresnelShlick(NdotH, diffuse, metalness);

    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;

    float3 numerator = NDF * G * F;
    float denum = 4.0f * NdotV * NdotL + 0.001f;

    float3 specular = numerator / denum;

    return (kD * diffuse + specular) * NdotL;
}


#endif // _LIGHTING_MODEL_H_
