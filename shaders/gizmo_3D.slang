#include "common/shaders_common.slang"
#include "common/global_uniform.slang"

#include "gizmo_3D_shared.slang"

#include "common/vertex/dre_vertex_layout.slang"

[[vk::binding(0, 3)]] ConstantBuffer<GizmoPassBuffer> cb;

struct PSInput
{
    [[vk::location(0)]] float3  wpos : POSITION;
    [[vk::location(1)]] float3  color : TEXCOORD0;
    [[vk::location(2)]] float3  normal : NORMAL;
    float4                      ndc_pos : SV_Position;
};


[shader("vertex")]
PSInput mainVS(VSInput input)
{
    PSInput out;

    out.wpos = mul(cb.m_Model, float4(input.pos, 1.0)).xyz;
    out.normal = input.norm;
    out.color = input.tan.xyz;

    float4 ndc_pos = mul(GetCameraViewProjM(), float4(out.wpos, 1.0));
    out.ndc_pos = ndc_pos;

    return out;
}

[shader("pixel")]
float4 mainPS(PSInput input) : SV_Target0
{
    float3 L = normalize(float3(1.0, 1.0, 0.0));
    float3 n = normalize(input.normal);

    float toWhite = dot(n, L) * 0.5 + 0.5;

    float3 white = float3(1.0, 1.0, 1.0);
    return float4(lerp(input.color, white, toWhite), 1.0);
}
