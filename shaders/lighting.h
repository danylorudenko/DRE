#ifndef _LIGHTING_H_
#define _LIGHTING_H_

#include "global_uniform.h"
#include "lighting_model.h"
#include "lights.h"
#include "shadows.h"

struct S_LIGHTING_RESULT
{
    vec3 finalRadiance;
};

struct S_SURFACE
{
    vec3 wpos;
    vec3 normal;
    vec3 diffuseSpectrum;
    float roughness;
    float metalness;
};

S_LIGHTING_RESULT CalculateLighting(S_SURFACE surface)
{
    S_LIGHTING_RESULT Result;
    Result.finalRadiance = vec3(0.0, 0.0, 0.0);

    //vec2 shadowUV = CalculateShadowUV(surface.wpos, GetSunShadowVP());

    //float CalculateShadow(in vec3 wpos, in mat4 shadowViewProj, vec2 shadowMapDims, in texture2D shadowMap)
    float shadow = CalculateShadow(surface.wpos, GetSunShadowVP(), GetSunShadowSize(), GetGlobalTexture(GetShadowMapID()));

    vec3 n = surface.normal;
    vec3 v = normalize(GetCameraPos() - surface.wpos);
    float NdotV = max(0.0, dot(n, v));

    uint lightsCount = GetLightsCount();
    for(uint i = 0; i < lightsCount; i++)
    {
        S_LIGHT_GPURef light = GetLight(i);
        switch(GetType(light))
        {
            case DRE_LIGHT_TYPE_SUN:
            {
                float shadow = CalculateShadow(surface.wpos, GetSunShadowVP(), GetSunShadowSize(), GetGlobalTexture(GetShadowMapID()));
                vec3 L = GetDirection(light);
                vec3 h = normalize(v + L);
                float NdotH = max(0.0, dot(n, h));
                float NdotL = max(0.0, dot(n, L));
                vec3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, surface.diffuseSpectrum, surface.roughness, surface.metalness);

                Result.finalRadiance += brdf * GetFlux(light) * shadow;
                break;
            }
            case DRE_LIGHT_TYPE_DIRECTIONAL:
            {
                vec3 L = GetDirection(light);
                vec3 h = normalize(v + L);
                float NdotH = max(0.0, dot(n, h));
                float NdotL = max(0.0, dot(n, L));
                vec3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, surface.diffuseSpectrum, surface.roughness, surface.metalness);

                Result.finalRadiance += brdf * GetFlux(light);
                break;
            }
            case DRE_LIGHT_TYPE_POINT:
            {
                break;
            }
        }
    }

    Result.finalRadiance += vec3(0.15, 0.15, 0.15) * surface.diffuseSpectrum; // simple ambient

    return Result;
}

#endif // _LIGHTING_H_
