#ifndef _DEBUG_PRIMITIVES_SLANG_
#define _DEBUG_PRIMITIVES_SLANG_

#include "common/shaders_common.slang"
#include "common/debug_draw.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DebugDrawCounters> counterBuffer;
[[vk::binding(1, 3)]] RWStructuredBuffer<DebugDrawBuffer> debugDrawBuffer;
[[vk::binding(2, 3)]] RWStructuredBuffer<DebugDrawBuffer> sortedDebugDrawBuffer;
[[vk::binding(3, 3)]] RWStructuredBuffer<DrawIndirectCommand> indirectArgs;

[shader("compute")]
[numthreads(64, 1, 1)]
void countCommandsCS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < debugDrawBuffer[0].commandCount)
    {
        uint argId = uint(debugDrawBuffer[0].commands[DTid.x].type);
        InterlockedAdd(counterBuffer[0].totalTypeCount[argId], 1);
    }
}

[shader("compute")]
[numthreads(64, 1, 1)]
void sortCommandsCS(uint3 DTid: SV_DispatchThreadID)
{
    if (DTid.x < debugDrawBuffer[0].commandCount)
    {
        uint argId = uint(debugDrawBuffer[0].commands[DTid.x].type);
        uint typeOffset = 0;
        for (uint i = 0; i < argId; ++i)
        {
            typeOffset += counterBuffer[0].totalTypeCount[i];
        }

        uint dstOffset;
        InterlockedAdd(counterBuffer[0].localSortingCounter[argId], 1, dstOffset);
        dstOffset += typeOffset;

        sortedDebugDrawBuffer[0].commands[dstOffset] = debugDrawBuffer[0].commands[DTid.x];
    }
}

[shader("compute")]
[numthreads(1, 1, 1)]
void fillIndirectArgsCS(uint3 DTid: SV_DispatchThreadID)
{
    sortedDebugDrawBuffer[0].commandCount = debugDrawBuffer[0].commandCount;

    uint firstInstance = 0;
    for (uint i = 0; i < uint(DebugDrawCommandType::COUNT); ++i)
    {
        //indirectArgs[i].indexCount    = (i == uint(DebugDrawCommandType::Line)) ? 2 : 0;
        indirectArgs[i].vertexCount = (i == uint(DebugDrawCommandType::Line)) ? 2 : 0;
        indirectArgs[i].instanceCount = counterBuffer[0].totalTypeCount[i];
        indirectArgs[i].firstInstance = firstInstance;
        indirectArgs[0].firstVertex = 0;
        firstInstance += counterBuffer[0].totalTypeCount[i];
    }
}


struct VSOutput
{
    float4                                      pos   : SV_Position;
    [[vk::location(0)]] nointerpolation uint    color : TEXCOORD0;
};

[shader("vertex")]
VSOutput mainVS(
    uint instanceID : SV_InstanceID,
    uint vertexID : SV_VertexID) // in vulkan instanceID starts from firstInstance, but in dx12 it's always from 0
{
    S_DEBUG_DRAW_COMMAND drawCommand = sortedDebugDrawBuffer[0].commands[instanceID];
    VSOutput output;
    output.pos = float4(0.0, 0.0, 0.0, 1.0);
    output.color = drawCommand.colorRGBA8;

    if (drawCommand.type == DebugDrawCommandType::Line)
    {
        float3 p0 = drawCommand.p0.xyz;
        float3 p1 = drawCommand.p1.xyz;
        if (vertexID == 0)
            output.pos = mul(GetCameraViewProjM(), float4(p0, 1.0));
        else
            output.pos = mul(GetCameraViewProjM(), float4(p1, 1.0));
    }

    return output;
}

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target0
{
    return unpackUnorm4x8ToFloat(input.color);
}


#endif // _DEBUG_PRIMITIVES_SLANG_
