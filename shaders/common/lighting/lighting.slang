#ifndef _LIGHTING_H_
#define _LIGHTING_H_

#include "common/shaders_common.slang"
#include "common/global_uniform.slang"
#include "common/lighting/lighting_model.slang"
#include "common/lighting/lights.slang"
#include "common/shadows/shadows.slang"
#include "common/global_illumination/ddgi_common.slang"

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
    Surface.ao = 1;

    Surface.prevWpos = wpos;

    return Surface;
}

S_LIGHTING_RESULT CalculateDirectLighting(S_SURFACE surface, float3 viewPosWS, ShaderStage shaderStage)
{
    S_LIGHTING_RESULT Result;
    Result.finalRadiance = float3(0.0f, 0.0f, 0.0f);

    float3 n = surface.normal;
    float3 v = normalize(viewPosWS - surface.wpos);
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

    return Result;
}

S_LIGHTING_RESULT CalculateDirectDiffuseLighting(S_SURFACE surface, ShaderStage shaderStage)
{
    S_LIGHTING_RESULT Result;
    Result.finalRadiance = float3(0.0f, 0.0f, 0.0f);

    float3 n = surface.normal;

    uint lightsCount = GetLightsCount();
    for(uint i = 0; i < lightsCount; i++)
    {
        S_LIGHT* light = GetLight(i);

        float3 L = GetDirection(light);
        float NdotL = saturate(dot(n, L) + 0.01);
        float3 brdf = surface.diffuseSpectrum * NdotL;
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

    //Result.finalRadiance += float3(0.15f, 0.15f, 0.15f) * surface.diffuseSpectrum * surface.ao; // simple ambient

    return Result;
}

S_LIGHTING_RESULT CalculateIndirectLighting(S_SURFACE surface, uint irradianceAtlasID, uint visibilityAtlasID, bool debugDrawCage = false)
{
    S_LIGHTING_RESULT Result;
    float3 accumulatedIrradiance = float3(0.0f, 0.0f, 0.0f);

    float weightsDebug[DDGIProbeNeighborhood::ProbeCount];

    DDGIProbeNeighborhood probes = GetProbeNeighborhood(surface.wpos, GetDDGI());
    float accumulatedWeight = 0.0;
    for (int i = 0; i < DDGIProbeNeighborhood::ProbeCount; i++)
    {
        uint probeIndex = probes.indices[i];
        float3 probeRadiance = SampleProbeRadiance(
            probeIndex,
            surface.normal,
            GetGlobalTextureObject(irradianceAtlasID),
            GetSamplerLinear(),
            GetDDGI().probeIrradianceResolution,
            GetDDGI().probeGridDimentions);

        float finalWeight = probes.weights[i];

        float3 pixelToProbe = surface.wpos - GetProbeWorldPosition(probeIndex, GetDDGI().probeWorldOffset, GetDDGI().probesWorldDistance.xyz, GetDDGI().probeGridDimentions);
        float statisticalVisibility = SampleProbeVisibility(
            length(pixelToProbe),
            probeIndex,
            surface.normal,
            GetGlobalTextureObject(visibilityAtlasID),
            GetSamplerLinear(),
            GetDDGI().probeVisibilityResolution,
            GetDDGI().probeGridDimentions);

        finalWeight *= statisticalVisibility;

        float3 pixelToProbeDir = -normalize(pixelToProbe);

        finalWeight *= saturate(dot(surface.normal, pixelToProbeDir));
        accumulatedWeight += finalWeight;
        accumulatedIrradiance += probeRadiance * finalWeight;

        weightsDebug[i] = finalWeight;
    }

    if (debugDrawCage)
    {
        for (int i = 0; i < DDGIProbeNeighborhood::ProbeCount; i++)
        {
            uint probeIndex = probes.indices[i];
            float3 probePos = GetProbeWorldPosition(probeIndex, GetDDGI().probeWorldOffset, GetDDGI().probesWorldDistance.xyz, GetDDGI().probeGridDimentions);

            float3 weightColor = lerp(float3(1, 0, 0), float3(0, 1, 0), weightsDebug[i] / accumulatedWeight);
            weightColor /= length(weightColor); // make it more visible

            if (weightsDebug[i] == 0.0)
                weightColor = float3(0, 0, 0);
            
            DrawDebugLine(surface.wpos, probePos, weightColor);
        }

    }

    accumulatedWeight = max(0.0001, accumulatedWeight);
    Result.finalRadiance = accumulatedIrradiance * surface.diffuseSpectrum * rcp(accumulatedWeight);
    return Result;
}

#endif // _LIGHTING_H_
