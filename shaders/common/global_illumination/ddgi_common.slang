#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.slang"

#define DDGI_VISUALIZATION_MODE_COLOR 0
#define DDGI_VISUALIZATION_MODE_NORMAL 1
#define DDGI_VISUALIZATION_MODE_UV 2

struct DDGIConstantBuffer
{
    uint3 probeGridDimentions;
    float probeDebugScale;

    float3 probesWorldDistance;
    int visualizationMode;

    float3 probeWorldOffset;
    float pad0;

    int probeGeometryVertexCount;
    int probeGeometryIndexCount;
    uint probeGBufferResolution; // hight and width of 1 probe in atlas
    uint probeVisibilityResolution; // hight and width of 1 probe in visibility atlas

    float irradianceUpdateRate;
};

struct DDGIProbeData
{
    float4 position;
};


#ifndef __cplusplus


uint GetProbeIndex(uint3 probeGridDimentions, uint3 DTid)
{
    uint sliceStart = probeGridDimentions.x * probeGridDimentions.y * DTid.z;
    uint columnStart = sliceStart + probeGridDimentions.x * DTid.y;
    uint probeIndex = columnStart + DTid.x;
    return probeIndex;
}

uint3 GetProbe3DIndex(uint3 probeGridDimentions, uint globalProbeIndex)
{
    uint slice = globalProbeIndex / (probeGridDimentions.x * probeGridDimentions.y);
    uint offsetInSlice = globalProbeIndex % (probeGridDimentions.x * probeGridDimentions.y);
    uint offsetInRow = offsetInSlice % probeGridDimentions.x;
    uint offsetInColumn = offsetInSlice / probeGridDimentions.x;

    return uint3(offsetInRow, offsetInColumn, slice);
}

uint2 GetProbeAtlasProbeStart(uint probeIndex, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint2 probeAtlasDimentions = probeResolution * uint2(probeGridDimentions.x, probeGridDimentions.y);
    probeAtlasDimentions.x *= probeGridDimentions.z;

    uint3 gridPos = GetProbe3DIndex(probeGridDimentions, probeIndex);

    uint2 topLeft = gridPos.xy * probeResolution;
    topLeft.x += gridPos.z * probeResolution.x;
    return topLeft;
}

uint4 GetDDGIProbeRect(uint probeIndex, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint2 probeStartCenter = GetProbeAtlasProbeStart(probeIndex, probeResolution, probeGridDimentions);
    uint2 probeEndCenter = probeStartCenter + probeResolution;
    return uint4(probeStartCenter, probeEndCenter);
}

uint3 GetProbe3DIndexFromPixel(uint2 atlasPixel, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint2 probeIndexInSlice = atlasPixel / probeResolution;
    uint sliceIndex = probeIndexInSlice.x / probeGridDimentions.x;
    uint2 probeIndexInGrid = uint2(probeIndexInSlice.x % probeGridDimentions.x, probeIndexInSlice.y);
    return uint3(probeIndexInGrid, sliceIndex);
}

uint GetProbeIndexFromPixel(uint2 atlasPixel, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint3 probe3DIndex = GetProbe3DIndexFromPixel(atlasPixel, probeResolution, probeGridDimentions);
    return GetProbeIndex(probeGridDimentions, probe3DIndex);
}

#endif // !__cplusplus

#endif // _DDGI_COMMON_H_