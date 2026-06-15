#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.slang"

#define DDGI_VISUALIZATION_MODE_COLOR 0
#define DDGI_VISUALIZATION_MODE_NORMAL 1
#define DDGI_VISUALIZATION_MODE_UV 2

struct DDGIConstantBuffer
{
    uint3 probesDimentions;
    float probeDebugScale;

    float3 probesWorldDistance;
    int visualizationMode;

    float3 probeWorldOffset;
    float pad0;

    int probeGeometryVertexCount;
    int probeGeometryIndexCount;
    uint probeGBufferResolution; // hight and width of 1 probe in atlas
    uint probeVisibilityResolution; // hight and width of 1 probe in visibility atlas
};

struct DDGIProbeData
{
    float4 position;
};


#ifndef __cplusplus


uint GetProbeIndex(DDGIConstantBuffer ddgiCB, uint3 DTid)
{
    uint sliceStart = ddgiCB.probesDimentions.x * ddgiCB.probesDimentions.y * DTid.z;
    uint columnStart = sliceStart + ddgiCB.probesDimentions.x * DTid.y;
    uint probeIndex = columnStart + DTid.x;
    return probeIndex;
}

uint2 GetProbeAtlasProbeStart(uint probeIndex, uint2 probeResolution, uint3 probeCounts)
{
    uint2 probeAtlasDimentions = probeResolution * uint2(probeCounts.x, probeCounts.y);
    probeAtlasDimentions.x *= probeCounts.z;

    uint slice = probeIndex / (probeCounts.x * probeCounts.y);
    uint offsetInSlice = probeIndex % (probeCounts.x * probeCounts.y);
    uint offsetInRow = offsetInSlice % probeCounts.x;
    uint offsetInColumn = offsetInSlice / probeCounts.x;

    uint2 topLeft = uint2(offsetInRow, offsetInColumn) * probeResolution;
    topLeft.x += slice * probeResolution.x;
    return topLeft;
}

uint4 GetDDGIProbeRect(uint probeIndex, uint2 probeResolution, uint3 probeCounts)
{
    uint2 probeStartCenter = GetProbeAtlasProbeStart(probeIndex, probeResolution, probeCounts);
    uint2 probeEndCenter = probeStartCenter + probeResolution;
    return uint4(probeStartCenter, probeEndCenter);
}

#endif // !__cplusplus

#endif // _DDGI_COMMON_H_