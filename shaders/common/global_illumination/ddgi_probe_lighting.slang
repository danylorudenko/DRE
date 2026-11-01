#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>        ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeSamples>      ddgiSampleBuffer;
[[vk::binding(2, 3)]] RWTexture2D                               ddgiAtlasIrradiance;
[[vk::binding(3, 3)]] Texture2D                                 ddgiAtlasIrradianceHistory;
[[vk::binding(4, 3)]] StructuredBuffer<DDGIProbeEncodedData>    ddgiProbeData;
[[vk::binding(5, 3)]] StructuredBuffer<uint>                    sortedProbeIndices;

// dispatched as (sortedSlot, probeTileX, probeTileY) workgroups
[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    uint sortedSlot = groupID.x;
    if (sortedSlot >= ddgiCB.totalProbeCount)
        return;

    uint2 probeResolution = ddgiCB.probeIrradianceResolution;
    uint2 pixelInProbe = groupID.yz * 8 + GTid.xy;
    if (any(pixelInProbe >= probeResolution))
        return;

    // test for the edge rows/columns
    if (any(pixelInProbe == 0) || any(pixelInProbe == probeResolution - 1))
    {
        return;
    }

    uint probeIndex = sortedProbeIndices[sortedSlot];
    uint2 atlasPixel = GetProbeAtlasProbeStart(probeIndex, probeResolution, ddgiCB.probeGridDimentions) + pixelInProbe;

    // probe was not traced this frame, carry history over to the current atlas
    if (sortedSlot >= ddgiCB.tracedProbeCount)
    {
        ddgiAtlasIrradiance[atlasPixel] = float4(ddgiAtlasIrradianceHistory[atlasPixel].rgb, 1.0f);
        return;
    }

    DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[probeIndex]);
    DDGIProbeSamples probeGBuffer = ddgiSampleBuffer[probeIndex];

    float4 uvRect = GetProbeAtlasUVRect(probeIndex, ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    float2 atlasUV = float2(atlasPixel + 0.5) / float2(GetProbeAtlasResolution(ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions));
    float2 probeUV = (atlasUV - uvRect.xy) / (uvRect.zw - uvRect.xy);

    float3 probePixelNormal = OctahedronDecode(probeUV);

    float3 accumulatedPixelRadiance = float3(0, 0, 0);
    float accumulatedWeight = 0;
    for (int i = 0; i < probeGBuffer.sampleCount; i++)
    {
        DDGISample sample = probeGBuffer.samples[i];
        float3 sampleDir = OctahedronDecode(sample.octDirection);
        float sampleWeight = saturate(dot(probePixelNormal, sampleDir));
        //sampleWeight = 1;
        //sample.radiance = float3(1, 1, 1);
        accumulatedPixelRadiance += sample.radiance * sampleWeight;
        accumulatedWeight += sampleWeight;
    }

    float3 resultPixelRadiance = ddgiAtlasIrradianceHistory[atlasPixel].rgb;
    if (accumulatedWeight > 0)
    {
        accumulatedPixelRadiance /= accumulatedWeight;
        resultPixelRadiance = accumulatedPixelRadiance * ddgiCB.updateRate + resultPixelRadiance * (1.0 - ddgiCB.updateRate);
    }

    ddgiAtlasIrradiance[atlasPixel] = float4(resultPixelRadiance, 1.0f);
}