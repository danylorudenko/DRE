#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DrawIndexedIndirectCommand>    indirectArgBuffer;
[[vk::binding(1, 3)]] StructuredBuffer<DDGIProbeData>                   probeData;
[[vk::binding(2, 3)]] ConstantBuffer<DDGIConstantBuffer>                ddgiCB;

float4 RemapDisplayValue(float4 value, float lowBound, float highBound)
{
    float distance = highBound - lowBound;
    return (value - lowBound) / distance;
}

[numthreads(4, 4, 4)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    //if (IsDDGIProbesDebugView(debugViewArgs))
    {
        uint probeIndex = GetProbeIndex(ddgiCB, DTid);

        // cleared by vkCmdFillBuffer
        //if (probeIndex == 0)
        //{
        //    indirectArgBuffer[0].indexCount = 3; // WRITE SPHERES VERTEX COUNT HERE
        //    indirectArgBuffer[0].instanceCount = 0;
        //    indirectArgBuffer[0].firstIndex = 0;
        //    indirectArgBuffer[0].vertexOffset = 0;
        //    indirectArgBuffer[0].firstInstance = 0;
        //}

        InterlockedAdd(indirectArgBuffer[0].instanceCount, 1u);
    }


}