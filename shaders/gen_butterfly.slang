#include "common/shaders_common.slang"

[[vk::binding(0, 3)]] RWTexture2D<float4> butterflyImage;

struct ButterflyData
{
    uint4 n;
    uint4 bit_reversed[256 / 4];
};
[[vk::binding(1, 3)]] ConstantBuffer<ButterflyData> butterflyData;

struct complex
{
    float r;
    float i;
};

uint GetBitReversedID(int i)
{
    return butterflyData.bit_reversed[i / 4][i % 4];
}

[numthreads(8, 8, 1)]
void mainCS(int2 DTid : SV_DispatchThreadID)
{
    int2 coords = int2(DTid.xy);
    uint N = butterflyData.n[0];

    int index = coords.y;
    int stage = coords.x;



    float k = ((index * N) / exp2(stage + 1)) % N;
    complex twiddle = complex(cos(2.0 * PI * k / float(N)) , sin(2.0 * PI * k / float(N)));
    int span = int(pow(2, stage));

    bool isTopOp = (index % exp2(stage + 1)) < exp2(stage);

    if (stage == 0)
    {
        if (isTopOp)
        {
            butterflyImage[coords] = float4(twiddle.r, twiddle.i, GetBitReversedID(index), GetBitReversedID(index + 1));
        }
        else
        {
        	butterflyImage[coords] = float4(twiddle.r, twiddle.i, GetBitReversedID(index - 1), GetBitReversedID(index));
        }
    }
    else
    {
        if (isTopOp)
        {
            butterflyImage[coords] = float4(twiddle.r, twiddle.i, index, index + span);
        }
        else
        {
            butterflyImage[coords] = float4(twiddle.r, twiddle.i, index - span, index);
        }
    }
}