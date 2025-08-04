#include "common/shaders_common.h"
#include "common/utils/poisson.h"

#ifndef _SHADOWS_H_
#define _SHADOWS_H_

vec2 CalculateShadowUV(in vec3 wpos, in mat4 shadowViewProj)
{
    vec3 lightspaceCoord = (shadowViewProj * vec4(wpos, 1.0)).xyz;
    vec2 shadowUV = lightspaceCoord.xy * 0.5 + 0.5;
    return shadowUV;
}

float ShadowMapSample(in vec3 wpos, in mat4 shadowViewProj, vec2 shadowMapDims, in texture2D shadowMap)
{
    vec3 lightspaceCoord = (shadowViewProj * vec4(wpos, 1.0)).xyz;
    vec2 shadowUV = lightspaceCoord.xy * 0.5 + 0.5;

#ifdef ENABLE_PCF
#ifdef ENABLE_PCF_POISSON
    float result = 0.0;
    float sampleCount = 16;
    for(int i = 0; i < sampleCount; i++)
    {
        float val = texture(sampler2D(shadowMap, GetSamplerLinear()), shadowUV + (poisson16[i] * 1) / shadowMapDims).r;
        result += val - 0.01 > lightspaceCoord.z ? 0.0 : 1.0;
    }
#else
    const int C_FILTER_SIZE = 2;
    vec2 start = floor(shadowUV * shadowMapDims);

    float result = 0.0;
    float sampleCount = C_FILTER_SIZE * C_FILTER_SIZE;
    for(int i = 0; i < C_FILTER_SIZE; i++)
    {
        for(int j = 0; j < C_FILTER_SIZE; j++)
        {
            float val = texture(sampler2D(shadowMap, GetSamplerNearest()), (start + vec2(i, j)) / shadowMapDims).r;
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

#ifndef DRE_VERTEX_SHADER
float ShadowVisibilityTrace(in vec3 wpos, in vec3 shadowDir)
{
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery,
        g_TLAS,
        gl_RayFlagsTerminateOnFirstHitEXT,
        0xFFFFFFFF,
        wpos,
        0.1f, // tMin
        shadowDir,
        10000); // tMax

    rayQueryProceedEXT(rayQuery);

    float result = 1.0;
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        result = 0.0;
    }

    return result;
}
#endif // DRE_VERTEX_SHADER

#endif // _SHADOWS_H_
