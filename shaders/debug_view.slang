#include "common/shaders_defines.slang"

struct DebugViewArgs
{
    // texture
    uint textureID;
    float sizeX;
    float sizeY;
    float offsetX;
    float offsetY;
    float lowBound;
    float highBound;
    uint channelMask;
};

#ifndef __cplusplus

uint GetChannelX(DebugViewArgs args)
{
    return args.channelMask & 0x1;
}

uint GetChannelY(DebugViewArgs args)
{
    return (args.channelMask >> 1) & 0x1;
}

uint GetChannelZ(DebugViewArgs args)
{
    return (args.channelMask >> 2) & 0x1;
}

uint GetChannelW(DebugViewArgs args)
{
    return (args.channelMask >> 3) & 0x1;
}

[[vk::binding(0, 3)]] RWTexture2D<float4> outputTexture;
[[vk::binding(1, 3)]] ConstantBuffer<DebugViewArgs> debugViewArgs;

#endif // !__cplusplus
