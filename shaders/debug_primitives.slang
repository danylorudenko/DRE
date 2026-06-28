#ifndef _DEBUG_PRIMITIVES_SLANG_
#define _DEBUG_PRIMITIVES_SLANG_

#include "common/shaders_defines.slang"
#include "common/debug_draw.slang"

struct VkDrawIndirectCommand
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

[[vk::binding(0, 3)]] RWStructuredBuffer<DebugDrawBuffer>       debugDrawBuffer;
[[vk::binding(1, 3)]] RWStructuredBuffer<VkDrawIndirectCommand> indirectArgs;

[shader("compute")]
[numthreads(1, 1, 1)]
void fillIndirectArgsCS(uint3 DTid : SV_DispatchThreadID)
{
    // TODO: translate debugDrawBuffer command list into indirect draw arguments
}

struct VSInput
{
    uint vertexID   : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    [[vk::location(0)]] float4 color : TEXCOORD0;
    float4                     pos   : SV_Position;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;
    output.pos   = float4(0.0, 0.0, 0.0, 1.0);
    output.color = float4(1.0, 1.0, 1.0, 1.0);
    // TODO: fetch line endpoints from debugDrawBuffer and output NDC position
    return output;
}

[shader("fragment")]
float4 mainPS(VSOutput input) : SV_Target0
{
    // TODO: output debug primitive color
    return input.color;
}

#endif // _DEBUG_PRIMITIVES_SLANG_
