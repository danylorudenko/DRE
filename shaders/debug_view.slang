#include "common/shaders_defines.slang"

#define DEBUG_VIEW_MODE_TEXTURE_BIT     (1 << 0)
#define DEBUG_VIEW_MODE_DDGI_PROBES_BIT (1 << 1)

struct DebugViewArgs
{
    uint DebugViewMode;

    // texture
    uint textureID;
    float size;
    float lowBound;
    float highBound;
    uint channelMask;

    // DDGI probes
    uint probeCount;
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

bool IsTextureDebugView(DebugViewArgs args)
{
    return (args.DebugViewMode & DEBUG_VIEW_MODE_TEXTURE_BIT) != 0;
}

bool IsDDGIProbesDebugView(DebugViewArgs args)
{
    return (args.DebugViewMode & DEBUG_VIEW_MODE_DDGI_PROBES_BIT) != 0;
}

[[vk::binding(0, 3)]] RWTexture2D<float4> outputTexture;
[[vk::binding(1, 3)]] ConstantBuffer<DebugViewArgs> debugViewArgs;

#endif // !__cplusplus
