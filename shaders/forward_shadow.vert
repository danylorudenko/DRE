#include "common/shaders_common.h"
#include "common/vertex/dre_vertex_layout.h"

struct PSInput
{
    [[vk::location(0)]] float3  wpos : POSITION;
    float4                      ndc_pos : SV_Position;
};

struct InstanceUniform
{
    float4x4  mvp_mat;
    float4x4  model_mat;
};
[[vk::binding(0, 3)]] ConstantBuffer<InstanceUniform> instanceUniform;

[shader("vertex")]
PSInput main(VSInput input)
{
    PSInput out;
    float4 pos = mul(instanceUniform.mvp_mat, float4(input.pos, 1.0));
    out.ndc_pos = pos;
    out.wpos = mul(instanceUniform.model_mat, float4(input.pos, 1.0)).xyz;

    return out;
}