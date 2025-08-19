#ifndef _LIGHTING_H_
#define _LIGHTING_H_

#include "common/shaders_common.h"
#include "common/global_uniform.h"
#include "common/lighting/lighting_model.h"
#include "common/lighting/lights.h"
#include "common/shadows/shadows.h"

struct S_LIGHTING_RESULT
{
    float3 finalRadiance;
};

struct S_SURFACE
{
    float3 wpos;
    float3 normal;
    float3 diffuseSpectrum;
    float  roughness;
    float  metalness;
    float4 prevWpos;
};

S_LIGHTING_RESULT CalculateLighting(S_SURFACE surface)
{
    S_LIGHTING_RESULT Result;
    Result.finalRadiance = float3(0.0f, 0.0f, 0.0f);

    float3 n = surface.normal;
    float3 v = normalize(GetCameraPos() - surface.wpos);
    float NdotV = max(0.0f, dot(n, v));

    uint lightsCount = GetLightsCount();
    for(uint i = 0; i < lightsCount; i++)
    {
        S_LIGHT* light = GetLight(i);
        switch(GetType(light))
        {
            case DRE_LIGHT_TYPE_SUN:
            {
                float3 L = GetDirection(light);
                float3 h = normalize(v + L);
                float NdotH = max(0.0f, dot(n, h));
                float NdotL = max(0.0f, dot(n, L));
                float3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, surface.diffuseSpectrum, surface.roughness, surface.metalness);

                #ifndef DRE_VERTEX_SHADER
                float shadow = ShadowVisibilityTrace(surface.wpos, L);
                #else
                float shadow = 1.0f;
                #endif // DRE_VERTEX_SHADER

                Result.finalRadiance += brdf * GetFlux(light) * shadow;
                break;
            }
            case DRE_LIGHT_TYPE_DIRECTIONAL:
            {
                float3 L = GetDirection(light);
                float3 h = normalize(v + L);
                float NdotH = max(0.0f, dot(n, h));
                float NdotL = max(0.0f, dot(n, L));
                float3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, surface.diffuseSpectrum, surface.roughness, surface.metalness);

                Result.finalRadiance += brdf * GetFlux(light);
                break;
            }
            case DRE_LIGHT_TYPE_POINT:
            {
                break;
            }
        }
    }

    Result.finalRadiance += float3(0.15f, 0.15f, 0.15f) * surface.diffuseSpectrum; // simple ambient

    return Result;
}

#endif // _LIGHTING_H_
