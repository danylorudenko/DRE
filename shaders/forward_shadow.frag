#include "common/shaders_common.h"

struct PSInput
{
    [[vk::location(0)]] float3  wpos : POSITION;
    float4                      ndc_pos : SV_Position;
};

struct PSOutput
{
    float4 wpos : SV_Target0;
};

[shader("pixel")]
PSOutput main(PSInput input)
{
    PSOutput out;
    out.wpos = float4(input.wpos.xyz, 1.0);

    return out;
}
