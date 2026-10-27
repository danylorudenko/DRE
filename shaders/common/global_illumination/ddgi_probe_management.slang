#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>        ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeEncodedData>  ddgiProbeData;

[shader("compute")]
[numthreads(4, 4, 4)]
void primeProbesCS(uint3 DTid : SV_DispatchThreadID)
{
    float3 probesCoordinateStart = -((float3(ddgiCB.probeGridDimentions) * ddgiCB.probesWorldDistance.xyz) / 2) + ddgiCB.probeWorldOffset;
    if (all(DTid < ddgiCB.probeGridDimentions))
    {
        uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);

        DDGIProbeData probeData;
        probeData.position = probesCoordinateStart + ddgiCB.probesWorldDistance.xyz * float3(DTid);
        probeData.framesSinceUpdate = SLANG_U16_MAX;
        probeData.isActive = true;
        probeData.updateWeight = PcgHash(probeIndex) * (1.0 / float(0xFFFFFFFFu)); // temporary random weights to test sorting
        probeData.irradianceMean = 0.0;

        ddgiProbeData[probeIndex] = EncodeProbeData(probeData);
    }
}

[shader("compute")]
[numthreads(4, 4, 4)]
void probeUpdateCS(uint3 DTid : SV_DispatchThreadID)
{
    if (all(DTid < ddgiCB.probeGridDimentions))
    {
        //uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);

        //DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[probeIndex]);
        //if (probeData.isActive)
        {
            //probeData.irradianceMean
        }


        //ddgiProbeData[probeIndex] = EncodeProbeData(probeData);
    }
}