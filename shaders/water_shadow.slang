#include "common/shaders_common.slang"
#include "common/vertex/dre_vertex_layout.slang"

struct InstanceUniform
{
    float4x4  mvp_mat;
};

[[vk::binding(0, 3)]] ConstantBuffer<InstanceUniform> instanceUniform;

[shader("vertex")]
float4 mainVS(DREVertex input) : SV_Position
{
    return mul(instanceUniform.mvp_mat, float4(input.pos, 1.0));
}