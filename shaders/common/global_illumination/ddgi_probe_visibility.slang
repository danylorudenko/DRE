#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>        ddgiCB;
[[vk::binding(1, 3)]] StructuredBuffer<DDGIProbeSamples>        ddgiSampleBuffer;
[[vk::binding(2, 3)]] RWTexture2D<float2>                       ddgiAtlasVisibility;
[[vk::binding(3, 3)]] Texture2D<float2>                         ddgiAtlasVisibilityHistory;
[[vk::binding(4, 3)]] StructuredBuffer<DDGIProbeEncodedData>    ddgiProbeData;
[[vk::binding(5, 3)]] StructuredBuffer<uint>                    sortedProbeIndices;

// dispatched as (sortedSlot, probeTileX, probeTileY) workgroups
[require(spvRayQueryKHR)]
[require(spvGroupNonUniformBallot)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    uint sortedSlot = groupID.x;
    if (sortedSlot >= ddgiCB.totalProbeCount)
        return;

    uint2 probeResolution = ddgiCB.probeVisibilityResolution;
    uint2 pixelInProbe = groupID.yz * 8 + GTid.xy;
    if (any(pixelInProbe >= probeResolution))
        return;

    // test for the edge rows/columns
    if (any(pixelInProbe == 0) || any(pixelInProbe == probeResolution - 1))
    {
        return;
    }

    uint probeIndex = WaveReadLaneFirst(sortedProbeIndices[sortedSlot]);
    uint2 atlasPixel = GetProbeAtlasProbeStart(probeIndex, probeResolution, ddgiCB.probeGridDimentions) + pixelInProbe;

    // probe was not traced this frame, carry history over to the current atlas
    if (sortedSlot >= ddgiCB.tracedProbeCount)
    {
        ddgiAtlasVisibility[atlasPixel] = ddgiAtlasVisibilityHistory[atlasPixel].xy;
        return;
    }

    DDGIProbeSamples probeGBuffer = ddgiSampleBuffer[probeIndex];

    float4 uvRect = GetProbeAtlasUVRect(probeIndex, ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions);
    float2 atlasUV = float2(atlasPixel + 0.5) / float2(GetProbeAtlasResolution(ddgiCB.probeVisibilityResolution, ddgiCB.probeGridDimentions));
    float2 probeUV = (atlasUV - uvRect.xy) / (uvRect.zw - uvRect.xy);

    float3 probePixelNormal = OctahedronDecode(probeUV);

    float2 accumulatedDistances = float2(0.0f, 0.0f); // distance and distance^2
    float accumulatedWeight = 0;

    for (int i = 0; i < probeGBuffer.sampleCount; i++)
    {
        DDGISample sample = probeGBuffer.samples[i];
        float sampleWeight = pow(saturate(dot(probePixelNormal, sample.direction)), 50.0f);

        float clampedDistance = min(sample.distance, GetVisibilityDistanceClamp(ddgiCB));

        accumulatedDistances += float2(clampedDistance, clampedDistance * clampedDistance) * sampleWeight;
        accumulatedWeight += sampleWeight;
    }

    float2 resultDistances = ddgiAtlasVisibilityHistory[atlasPixel].xy;
    if (accumulatedWeight > 0)
    {
        accumulatedDistances /= accumulatedWeight;

        resultDistances = accumulatedDistances * ddgiCB.updateRate + resultDistances * (1.0 - ddgiCB.updateRate);
    }

    ddgiAtlasVisibility[atlasPixel] = resultDistances;

}