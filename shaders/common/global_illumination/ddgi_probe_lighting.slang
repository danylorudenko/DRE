#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeSamples>  ddgiSampleBuffer;
[[vk::binding(2, 3)]] RWTexture2D                           ddgiAtlasIrradiance;
[[vk::binding(3, 3)]] Texture2D                             ddgiAtlasIrradianceHistory;
[[vk::binding(4, 3)]] StructuredBuffer<DDGIProbeData>       ddgiProbeData;

[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    uint2 atlasResolution = GetProbeAtlasResolution(ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    if (any(DTid.xy >= atlasResolution))
        return;

    uint probeIndex = GetProbeIndexFromPixel(DTid.xy, ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    uint4 probeRect = GetDDGIProbeRect(probeIndex, ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    if (any(DTid.xy < probeRect.xy) || any(DTid.xy >= probeRect.zw))
        return; // hmmmm

    DDGIProbeData probeData = ddgiProbeData[probeIndex];
    DDGIProbeSamples probeGBuffer = ddgiSampleBuffer[probeIndex];

    float4 uvRect = GetProbeAtlasUVRect(probeIndex, ddgiCB.probeIrradianceResolution, ddgiCB.probeGridDimentions);
    uint2 atlasPixel = DTid.xy;
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

    if (accumulatedWeight > 0)
    {
        accumulatedPixelRadiance /= accumulatedWeight;
    }

    ddgiAtlasIrradiance[atlasPixel] = float4(accumulatedPixelRadiance * ddgiCB.irradianceUpdateRate + ddgiAtlasIrradianceHistory[atlasPixel].rgb * (1.0 - ddgiCB.irradianceUpdateRate), 1.0f);

}