#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeSamples>  ddgiSampleBuffer;
[[vk::binding(2, 3)]] RWTexture2D<float2>                   ddgiAtlasVisibility;
[[vk::binding(3, 3)]] Texture2D<float2>                     ddgiAtlasVisibilityHistory;
[[vk::binding(4, 3)]] StructuredBuffer<DDGIProbeData>       ddgiProbeData;

[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    uint2 atlasResolution = GetProbeAtlasResolution(ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);
    if (any(DTid.xy >= atlasResolution))
        return;

    uint probeIndex = GetProbeIndexFromPixel(DTid.xy, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);

    // returns full rect (borders included)
    uint4 probeRect = GetDDGIProbeRect(probeIndex, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);

    // test for the edge rows/columns
    if (any(DTid.xy == probeRect.xy) || any(DTid.xy == probeRect.zw))
        return; // hmmmm

    DDGIProbeData probeData = ddgiProbeData[probeIndex];
    DDGIProbeSamples probeGBuffer = ddgiSampleBuffer[probeIndex];

    float4 uvRect = GetProbeAtlasUVRect(probeIndex, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);
    uint2 atlasPixel = DTid.xy;
    float2 atlasUV = float2(atlasPixel + 0.5) / float2(GetProbeAtlasResolution(ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions));
    float2 probeUV = (atlasUV - uvRect.xy) / (uvRect.zw - uvRect.xy);

    float3 probePixelNormal = OctahedronDecode(probeUV);

    float2 accumulatedDistances = 0.0f; // distance and distance^2
    float accumulatedWeight = 0;

    for (int i = 0; i < probeGBuffer.sampleCount; i++)
    {
        DDGISample sample = probeGBuffer.samples[i];
        float3 sampleDir = OctahedronDecode(sample.octDirection);
        float sampleWeight = saturate(dot(probePixelNormal, sampleDir));

        float clampedDistance = min(sample.distance, ddgiCB.probesWorldDistance.w * 2);

        accumulatedDistances += float2(clampedDistance, clampedDistance * clampedDistance) * sampleWeight;
        accumulatedWeight += sampleWeight;
    }

    float2 resultDistances = ddgiAtlasVisibilityHistory[atlasPixel].xy;
    if (accumulatedWeight > 0)
    {
        accumulatedDistances /= accumulatedWeight;

        resultDistances = accumulatedDistances * ddgiCB.updateRate + resultDistances * (1.0 - ddgiCB.updateRate);
        //resultDistances = max(resultDistances, accumulatedDistances);
    }

    ddgiAtlasVisibility[atlasPixel] = resultDistances;

}