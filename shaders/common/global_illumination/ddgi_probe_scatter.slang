#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;
[[vk::binding(1, 3)]] RWStructuredBuffer<DDGIProbeData>     ddgiProbesData;


[shader("compute")]
[numthreads(4, 4, 4)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 probesCoordinateStart = -((float3(ddgiCB.probesDimentions) * ddgiCB.probesWorldDistance) / 2);
    if (all(DTid < ddgiCB.probesDimentions))
    {
        uint probeIndex = GetProbeIndex(ddgiCB, DTid);

        DDGIProbeData probeData;
        probeData.position = float4(probesCoordinateStart + ddgiCB.probesWorldDistance * float3(DTid), 1.0);
        
        ddgiProbesData[probeIndex] = probeData;
    }
}