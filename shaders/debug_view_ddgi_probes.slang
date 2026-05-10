#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/vertex/dre_vertex_layout.slang"
#include "common/gbuffer.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DrawIndexedIndirectCommand>    indirectArgBuffer;
[[vk::binding(1, 3)]] StructuredBuffer<DDGIProbeData>                   probeData;
[[vk::binding(2, 3)]] ConstantBuffer<DDGIConstantBuffer>                ddgiCB;

float4 RemapDisplayValue(float4 value, float lowBound, float highBound)
{
    float distance = highBound - lowBound;
    return (value - lowBound) / distance;
}

[numthreads(4, 4, 4)]
void indirectArgsFillCS(uint3 DTid : SV_DispatchThreadID)
{
    //if (IsDDGIProbesDebugView(debugViewArgs))
    {
        uint probeIndex = GetProbeIndex(ddgiCB, DTid);
        InterlockedAdd(indirectArgBuffer[0].instanceCount, 1u);
    }
}


struct PSInput
{
    float4 ndc_pos : SV_POSITION;
    float3 wpos : WORLDPOSITION;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD0;
    nointerpolation uint instanceID : TEXCOORD1;
};

[shader("vertex")]
PSInput drawVS(VSInput input, uint instanceID : SV_InstanceID)
{
    PSInput output;
    output.wpos = input.pos * ddgiCB.probeDebugScale;

    DDGIProbeData probeData = probeData[instanceID];
    output.wpos += probeData.position.xyz;

    float4 ndc_pos = mul(GetCameraViewProjM(), float4(output.wpos, 1.0));
    ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter

    output.ndc_pos = ndc_pos;

    output.norm = input.norm;
    output.uv = input.uv;
    output.instanceID = instanceID;

    return output;
}

// generate static array of random colors here
static const float3 debugColors[8] =
{
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 0.0, 1.0),
    float3(0.0, 1.0, 1.0),
    float3(1.0, 0.5, 0.0),
    float3(0.5, 0.0, 1.0)
};

[shader("pixel")]
GBuffer drawPS(PSInput input)
{

    GBuffer Output;
    float3 probeColor = debugColors[input.instanceID % 8];
    Output.Diffuse_Roughness = float4(probeColor, 0.0);

    float3 encodedNormal = normalize(input.norm) * 0.5 + 0.5;
    Output.Normal_Metalness = float4(encodedNormal, 0.0);

    Output.Velocity = CalculateVelocity(input.ndc_pos, input.wpos);
    //Output.ObjectID = GlobalID2Color();

    //Output.DEBUG_TEXTURE = float4(input.TBN[2], 1);
    //Output.DEBUG_TEXTURE = float4(frac(input.uv), 0, 1);
    Output.DEBUG_TEXTURE = float4(1, 0, 0, 1);
    
    return Output;
}