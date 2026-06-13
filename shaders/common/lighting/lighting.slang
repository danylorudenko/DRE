#ifndef _LIGHTING_H_
#define _LIGHTING_H_

#include "common/shaders_common.slang"
#include "common/global_uniform.slang"
#include "common/lighting/lighting_model.slang"
#include "common/lighting/lights.slang"
#include "common/shadows/shadows.slang"

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

    float  ao;

    float3 prevWpos;
};

S_SURFACE InitSurface(float3 wpos)
{
    S_SURFACE Surface;
    Surface.wpos = wpos;
    Surface.normal = float3(0,0,1);
    Surface.diffuseSpectrum = float3(1,1,1);
    Surface.roughness = 1;
    Surface.metalness = 0;
    Surface.ao = 0;

    Surface.prevWpos = wpos;

    return Surface;
}

S_LIGHTING_RESULT CalculateDirectLighting(S_SURFACE surface, ShaderStage shaderStage)
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

        float3 L = GetDirection(light);
        float3 h = normalize(v + L);
        float NdotH = max(0.0f, dot(n, h));
        float NdotL = max(0.0f, dot(n, L));
        float3 brdf = CookTorranceBRDF(NdotH, NdotV, NdotL, surface.diffuseSpectrum, surface.roughness, surface.metalness);
        float3 directRadiance = brdf * GetFlux(light);
        float shadow = 1;

        switch(GetType(light))
        {
            case DRE_LIGHT_TYPE_SUN:
            case DRE_LIGHT_TYPE_DIRECTIONAL:
            {
            if (shaderStage != ShaderStage::Vertex)
            {
                shadow = ShadowVisibilityTrace(surface.wpos, L);
                break;
            }
            }
        }

        directRadiance *= shadow;

        Result.finalRadiance += directRadiance;
    }

    Result.finalRadiance += float3(0.15f, 0.15f, 0.15f) * surface.diffuseSpectrum * surface.ao; // simple ambient

    return Result;
}

#endif // _LIGHTING_H_
