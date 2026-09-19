#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>        ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeEncodedData>  ddgiProbesData;


[shader("compute")]
[numthreads(4, 4, 4)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    float3 probesCoordinateStart = -((float3(ddgiCB.probeGridDimentions) * ddgiCB.probesWorldDistance.xyz) / 2) + ddgiCB.probeWorldOffset;
    if (all(DTid < ddgiCB.probeGridDimentions))
    {
        uint probeIndex = GetProbeIndex(ddgiCB.probeGridDimentions, DTid);

        DDGIProbeData probeData = DecodeProbeData(ddgiProbesData[probeIndex]);
        probeData.position = probesCoordinateStart + ddgiCB.probesWorldDistance.xyz * float3(DTid);
        probeData.framesSinceUpdate += 1;

        ddgiProbesData[probeIndex] = EncodeProbeData(probeData);
    }
}