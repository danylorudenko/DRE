#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.h"

struct DDGIConstantBuffer
{
    uint3 probesDimentions;
    int pad0;
    float3 probesWorldDistance;
    int pad1;
};

struct DDGIProbeData
{
    float4 position;
};

#endif // _DDGI_COMMON_H_