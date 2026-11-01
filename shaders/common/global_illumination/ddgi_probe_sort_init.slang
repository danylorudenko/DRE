#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<uint> probeIndices;
[[vk::binding(1, 3)]] ConstantBuffer<DDGIConstantBuffer> ddgiCB;

// fills identity probe indices, which are then sorted by probe weight
[shader("compute")]
[numthreads(64, 1, 1)]
void initIndicesCS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < ddgiCB.totalProbeCount)
    {
        probeIndices[DTid.x] = DTid.x;
    }
}
