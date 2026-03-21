#include "common/shaders_defines.h"

struct DebugViewArgs
{
    uint textureID;
    float size;
    float lowBound;
    float highBound;
    uint channelMask;
};

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
