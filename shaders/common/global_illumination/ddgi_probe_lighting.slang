#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>        ddgiCB;
[[vk::binding(1, 3)]] StructuredBuffer<DDGIProbeSamples>        ddgiSampleBuffer;
[[vk::binding(2, 3)]] RWTexture2D                               ddgiAtlasIrradiance;
[[vk::binding(3, 3)]] Texture2D                                 ddgiAtlasIrradianceHistory;
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

    uint2 probeResolution = ddgiCB.probeIrradianceResolution;
    uint2 targetPixel = groupID.yz * 8 + GTid.xy;
    if (any(targetPixel >= probeResolution))
        return;

    uint2 sourcePixel = GetSourcePixelForBorderLocal(targetPixel, ddgiCB.probeIrradianceResolution);

    uint probeIndex = WaveReadLaneFirst(sortedProbeIndices[sortedSlot]);

    uint2 probeStart = GetProbeAtlasProbeStart(probeIndex, probeResolution, ddgiCB.probeGridDimentions);
    uint2 sourceAtlasPixel = probeStart + sourcePixel;
    uint2 targetAtlasPixel = probeStart + targetPixel;

    // probe was not traced this frame, carry history over to the current atlas
    if (sortedSlot >= ddgiCB.tracedProbeCount)
    {
        ddgiAtlasIrradiance[targetAtlasPixel] = float4(ddgiAtlasIrradianceHistory[sourceAtlasPixel].rgb, 1.0f);
        return;
    }

    DDGIProbeSamples probeGBuffer = ddgiSampleBuffer[probeIndex];

    float4 uvRect = GetProbeAtlasUVRect(probeIndex, ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    float2 atlasUV = float2(sourceAtlasPixel + 0.5) / float2(GetProbeAtlasResolution(ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions));
    float2 probeUV = (atlasUV - uvRect.xy) / (uvRect.zw - uvRect.xy);

    float3 probePixelNormal = OctahedronDecode(probeUV);

    float3 accumulatedPixelRadiance = float3(0, 0, 0);
    float accumulatedWeight = 0;
    for (int i = 0; i < probeGBuffer.sampleCount; i++)
    {
        DDGISample sample = probeGBuffer.samples[i];
        float sampleWeight = saturate(dot(probePixelNormal, sample.direction));
        accumulatedPixelRadiance += sample.radiance * sampleWeight;
        accumulatedWeight += sampleWeight;
    }

    float3 resultPixelRadiance = ddgiAtlasIrradianceHistory[targetAtlasPixel].rgb;
    if (accumulatedWeight > 0)
    {
        accumulatedPixelRadiance /= accumulatedWeight;
        resultPixelRadiance = accumulatedPixelRadiance * ddgiCB.updateRate + resultPixelRadiance * (1.0 - ddgiCB.updateRate);
    }

    ddgiAtlasIrradiance[targetAtlasPixel] = float4(resultPixelRadiance, 1.0f);
}