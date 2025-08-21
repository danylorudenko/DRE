#include "common/shaders_common.h"

struct ImGuiData
{
    float4 pos_screenSize;
    uint textureID;
};
[[vk::binding(0, 3)]] ConstantBuffer<ImGuiData> imGuiData;

struct PSIn
{
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float4 color : TEXCOORD1;
};

[shader("pixel")]
float4 main(PSIn in) : SV_Target0
{
	float textureValue = SampleGlobalTextureLinear(imGuiData.textureID, in.uv).x;
    return float4(in.color.rgba * textureValue);
}
