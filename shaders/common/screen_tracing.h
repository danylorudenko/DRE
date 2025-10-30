#ifndef __SCREEN_TRACING_H__
#define __SCREEN_TRACING_H__

#include "common\shaders_common.h"

struct ScreenTracingParamsOBSOLETE
{
    float2  centerUV;
    float2  stepUV;
    int     maxStepCount;
    float3  surfacePositionWS;
    float3  surfaceNormalWS;
};

// this is stupid, not sure I'll need this form of tracing
// returns UV of the sample on intersection
float2 TraceScreenDepthForIntersection(ScreenTracingParamsOBSOLETE tracingParams, Texture2D depthTexture)
{
    float2 stepUV = tracingParams.stepUV;
    float2 center = tracingParams.centerUV;

    for (int step = 1; step < tracingParams.maxStepCount; step++)
    {
        float2 marchUV = center + stepUV * step;
        float deviceZSample = depthTexture.SampleLevel(GetSamplerNearestClamp(), marchUV, 0).x;

        float zView = ConvertDeviceZToViewZ(deviceZSample);

        float3 marchedPosWS = ConvertUVzViewToWorld(marchUV, zView);
        //if (distance(marchWorldPos, centerWorldPos) > GetMaxOcclusionDistance())
        //{
        //    continue;
        //}

        float3 marchedSampleDirection = normalize(marchedPosWS - tracingParams.surfacePositionWS);
        float horizonAngle = dot(marchedSampleDirection, tracingParams.surfaceNormalWS);
        if (horizonAngle >= 0.001)
        {
            return marchUV;
        }
    }

    return center + stepUV * tracingParams.maxStepCount;
}


struct ScreenTracingParams
{
    float2  surfacePosUV;
    float3  surfacePosVS;
    float3  surfaceNormalVS;
    float3  traceDirVS;
    float   traceDistanceVS;
    int     maxStepsCount;
};

// returns ange of the horizon
float TraceScreenDepthForHorizon(ScreenTracingParams tracingParams, Texture2D<float> depthTexture)
{
    float2 center = tracingParams.surfacePosUV;
    float4 traceDirClip = mul(GetCameraProjM(), float4(tracingParams.traceDirVS * tracingParams.traceDistanceVS, 0));
    float3 traceStepNDC = (traceDirClip.xyz / traceDirClip.w) / tracingParams.maxStepsCount;
    float2 traceStepUV = traceStepNDC.xy * 0.5 - 0.5;

    float horizonCos = -1;
    for (int step = 1; step < tracingParams.maxStepsCount; step++)
    {
        float2 marchUV = center + traceStepUV * step;
        float deviceZSample = depthTexture.SampleLevel(GetSamplerNearestClamp(), marchUV, 0).x;

        float zView = ConvertDeviceZToViewZ(deviceZSample);

        float3 marchedPosWS = ConvertUVzViewToWorld(marchUV, zView);
        float3 marchedPosVS = ConvertUVzViewToView(marchUV, zView);
        //if (distance(marchWorldPos, centerWorldPos) > GetMaxOcclusionDistance())
        //{
        //    continue;
        //}

        float3 marchedSampleDirectionVS = normalize(marchedPosVS - tracingParams.surfacePosVS);
        float sampleHorizonAngle = dot(marchedSampleDirectionVS, tracingParams.surfaceNormalVS);
        horizonCos = max(horizonCos, sampleHorizonAngle);
    }

    return acos(horizonCos);
}

#endif // __SCREEN_TRACING_H__