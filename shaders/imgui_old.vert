#include "common/shaders_common.h"

struct VSIn
{
    [[vk::location(0)]] float2 pos : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float4 color : TEXCOORD1;
};

struct PSIn
{
    [[vk::location(0)]] float2  uv : TEXCOORD0;
    [[vk::location(1)]] float4  color : TEXCOORD1;
    float4                      ndc_pos : SV_Position;
};

struct ImGuiData
{
    float4 pos_screenSize;
    uint textureID;
};
[[vk::binding(0, 3)]] ConstantBuffer<ImGuiData> imGuiData;

[shader("vertex")]
PSIn main(VSIn input)
{
    PSIn out;
    out.uv = input.uv;
    out.color = input.color;

    float2 scale = float2(2.0 / imGuiData.pos_screenSize.z, 2.0 / imGuiData.pos_screenSize.w);
    float2 translate = float2(-1.0 - imGuiData.pos_screenSize.x * scale.x, -1.0 - imGuiData.pos_screenSize.y * scale.y);

    out.ndc_pos = float4(input.pos.x * scale.x + translate.x, input.pos.y * scale.y + translate.y, 0.5, 1.0);

    return out;
}