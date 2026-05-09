#include "common/shaders_common.slang"
#include "common/utils/poisson.slang"

#ifndef _SHADOWS_H_
#define _SHADOWS_H_

float2 CalculateShadowUV(float3 wpos, float4x4 shadowViewProj)
{
    float3 lightspaceCoord = mul(shadowViewProj, float4(wpos, 1.0f)).xyz;
    float2 shadowUV = lightspaceCoord.xy * 0.5f + 0.5f;
    return shadowUV;
}

float ShadowMapSample(float3 wpos, float4x4 shadowViewProj, float2 shadowMapDims, Texture2D<float> shadowMap)
{
    float3 lightspaceCoord = mul(shadowViewProj, float4(wpos, 1.0f)).xyz;
    float2 shadowUV = lightspaceCoord.xy * 0.5f + 0.5f;

#ifdef ENABLE_PCF
#ifdef ENABLE_PCF_POISSON
    float result = 0.0f;
    int sampleCount = 16;
    for(int i = 0; i < sampleCount; i++)
    {
        float val = shadowMap.Sample(GetSamplerLinear(), shadowUV + (poisson16[i] * 1) / shadowMapDims).r;
        result += val - 0.01f > lightspaceCoord.z ? 0.0f : 1.0f;
    }
#else
      const int C_FILTER_SIZE = 2;
      float2 start = floor(shadowUV * shadowMapDims);

      float result = 0.0f;
      int sampleCount = C_FILTER_SIZE * C_FILTER_SIZE;
      for(int i = 0; i < C_FILTER_SIZE; i++)
      {
          for(int j = 0; j < C_FILTER_SIZE; j++)
          {
              float val = shadowMap.Sample(GetSamplerNearest(), (start + float2(i, j)) / shadowMapDims).r;
              result += val - 0.01f > lightspaceCoord.z ? 0.0f : 1.0f;
          }
      }
#endif
    result /= sampleCount;

#else
      float shadowValue = shadowMap.Sample(GetSamplerNearest(), shadowUV).r;
      float result = shadowValue - 0.01f > lightspaceCoord.z ? 0.0f : 1.0f;
#endif

    return result;
}

float ShadowVisibilityTrace(float3 wpos, float3 shadowDir)
{
    RayDesc rayDesc;
    rayDesc.Origin = wpos;
    rayDesc.TMin = 0.1;
    rayDesc.Direction = shadowDir;
    rayDesc.TMax = 10000;

    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | 
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | 
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;

    rayQuery.TraceRayInline(
        g_TLAS,
        RAY_FLAG_NONE, // or'ed with RayQuery<..>
        0xFFFFFFFF,
        rayDesc);

    rayQuery.Proceed();

    float result = 1.0f;
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        result = 0.0f;
    }

    return result;
}

#endif // _SHADOWS_H_

