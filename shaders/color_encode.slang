#include "common/shaders_common.slang"

[[vk::binding(0, 3)]] RWTexture2D<float4>      linearRT;
[[vk::binding(1, 3)]] RWTexture2D<float4>    encodedOutput;

struct PassUniform
{
    float4 useEncoded_exposure;
};
[[vk::binding(2, 3)]] ConstantBuffer<PassUniform> passUniform;

float GetUseEncoded() { return passUniform.useEncoded_exposure.x; }
float GetExposure() { return passUniform.useEncoded_exposure.y; }


float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    int2 coords = int2(DTid.xy);

    float3 linearColor = linearRT[coords].rgb;
    float3 encoded = ACESFilm(linearColor * GetExposure());

    float3 result = lerp(linearColor, encoded, GetUseEncoded());

    encodedOutput[coords] = float4(result, 1.0);
}