#include "common/shaders_common.h"
#include "common/utils/poisson.h"
#include "common/lighting/lighting_model.h"

struct PSInput
{
    float4                       ndc_pos   : SV_Position;
    [[vk::location(0)]] float3  ray_start : TEXCOORD0;
    [[vk::location(1)]] float3  ray_end   : TEXCOORD1;
};

struct InstanceUniform
{
    float4x4 model_mat;
    float4x4 prev_model_mat;
    uint4    textureID;
};
[[vk::binding(0, 4)]] ConstantBuffer<InstanceUniform> instanceUniform;

[shader("pixel")]
float main(PSInput input) : SV_Target0
{
    float old_area = length(ddx(input.ray_start) * ddy(input.ray_start));
    float new_area = length(ddx(input.ray_end) * ddy(input.ray_end));

    float ratio = old_area / new_area;

    return ratio * 0.3;
}

