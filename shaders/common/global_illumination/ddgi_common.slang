#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.slang"

struct DDGIConstantBuffer
{
    uint3 probesDimentions;
    float probeDebugScale;
    float3 probesWorldDistance;
    int pad1;
};

struct DDGIProbeData
{
    float4 position;
};

#ifndef __cplusplus
uint GetProbeIndex(DDGIConstantBuffer ddgiCB, uint3 DTid)
{
    uint sliceStart = ddgiCB.probesDimentions.x * ddgiCB.probesDimentions.y * DTid.z;
    uint columnStart = sliceStart + ddgiCB.probesDimentions.y * DTid.y;
    uint probeIndex = columnStart + DTid.x;
    return probeIndex;
}
#endif // !__cplusplus

#endif // _DDGI_COMMON_H_