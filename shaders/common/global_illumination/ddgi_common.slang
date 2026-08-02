#ifndef _DDGI_COMMON_H_
#define _DDGI_COMMON_H_

#include "common/shaders_defines.slang"

#ifndef __cplusplus
#include "common/utils/octahedron.slang"
#endif // __cplusplus

#define DDGI_MAX_SAMPLES_PER_PROBE 64

#define DDGI_IRRADIANCE_PROBE_RESOLUTION 6
#define DDGI_VISIBILITY_PROBE_RESOLUTION 16

#define DDGI_VISUALIZATION_MODE_COLOR 0
#define DDGI_VISUALIZATION_MODE_NORMAL 1
#define DDGI_VISUALIZATION_MODE_UV 2

#define DDGI_RAY_DISTRIBUTION_STABLE 0
#define DDGI_RAY_DISTRIBUTION_RANDOM 1

struct DDGIConstantBuffer
{
    uint3 probeGridDimentions;
    float probeDebugScale;

    float4 probesWorldDistance; // w - max from x,y,z

    float3 probeWorldOffset;
    uint rayCountPerProbe;

    int probeGeometryVertexCount;
    int probeGeometryIndexCount;
    uint probeIrradianceResolution; // hight and width of 1 probe in irradiance atlas
    uint probeVisibilityResolution; // hight and width of 1 probe in visibility atlas

    uint irradianceAtlasTextureID;
    uint irradianceHistoryTextureID;
    uint visibilityAtlasTextureID;
    uint visibilityHistoryTextureID;

    float updateRate;
    uint rayDistributionMode; // DDGI_RAY_DISTRIBUTION_*
    uint visualizationMode;
    uint drawSelectedCage;

    uint3 debugProbeFocus;
    uint debugDrawRays;

};

struct DDGIProbeData
{
    float4 position;
};

struct DDGISample
{
    float3 radiance;
    float distance;
    float2 octDirection;
};
/*
DDGISampleEncoded EncodeDDGISample(float3 probeLocalPosition, float3 diffuse, float2 octNormal, float roughness, float metalness)
{
    DDGISampleEncoded sample;
    sample.probeLocalPosition = half3(probeLocalPosition);
    sample.data0 = pack_clamp_u8(uint4(diffuse * 255.0f, roughness * 255.0f));
    sample.data1 = pack_clamp_u8(uint4(octNormal * 255.0, metalness * 255.0f, 0));
    return sample;
}

DDGISample DecodeDDGISample(DDGISampleEncoded encodedSample)
{
    float4 diffuse_roughness = (float4)encodedSample.data0 / 255.0f;
    float3 octNormal_metalness = (float3)encodedSample.data1.xyz / 255.0f;

    DDGISample sample;
    sample.probeLocalPosition = float3(encodedSample.probeLocalPosition);
    sample.diffuse = diffuse_roughness.rgb;
    sample.roughness = diffuse_roughness.a;
    sample.octNormal = octNormal_metalness.xy;
    sample.metalness = octNormal_metalness.z;

    return sample;
}
*/
struct DDGIProbeSamples
{
    uint sampleCount;
    DDGISample samples[DDGI_MAX_SAMPLES_PER_PROBE];
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
    uint3 gridPos = GetProbe3DIndex(probeGridDimentions, probeIndex);

    uint2 topLeft = gridPos.xy * probeResolution;
    topLeft.x += gridPos.z * probeGridDimentions.x * probeResolution.x;
    return topLeft;
}

// includes border pixels, returns full rect
uint4 GetDDGIProbeRect(uint probeIndex, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint2 probeStart = GetProbeAtlasProbeStart(probeIndex, probeResolution, probeGridDimentions);
    uint2 probeEnd = probeStart + probeResolution;
    return uint4(probeStart, probeEnd);
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

uint2 GetProbeAtlasResolution(uint2 probeResolution, uint3 probeGridDimentions)
{
    return probeResolution * uint2(probeGridDimentions.x, probeGridDimentions.y) * uint2(probeGridDimentions.z, 1);
}

// does not include border pixels, returns only inner rect
float4 GetProbeAtlasUVRect(uint probeIndex, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint4 probeRect = GetDDGIProbeRect(probeIndex, probeResolution, probeGridDimentions);
    uint2 atlasSize = GetProbeAtlasResolution(probeResolution, probeGridDimentions);

    probeRect += uint4(1, 1, -1, -1); // exclude border pixels from uv rect
    float2 uvStart = probeRect.xy / (float2)atlasSize;
    float2 uvEnd = probeRect.zw / (float2)atlasSize;
    return float4(uvStart, uvEnd);
}

uint2 CalculateDDGIAtlasTargetPixel(uint probeIndex, float2 probeLocalUV, uint2 probeResolution, uint3 probeGridDimentions)
{
    uint4 probeGBufferRect = GetDDGIProbeRect(probeIndex, probeResolution, probeGridDimentions);
    uint2 probeInnerRes = probeResolution - 2;
    uint2 innerStart = probeGBufferRect.xy + 1;
    uint2 targetPixel = innerStart + uint2(probeLocalUV * probeInnerRes); // maybe needs a clamp to innerRes

    return targetPixel;
}

float3 GetProbeWorldPosition(uint probeIndex, float3 probeWorldOffset, float3 probesWorldDistance, uint3 probeGridDimentions)
{
    uint3 probe3DIndex = GetProbe3DIndex(probeGridDimentions, probeIndex);
    float3 probesCoordinateStart = -((float3(probeGridDimentions) * probesWorldDistance) / 2.0f) + probeWorldOffset;
    return probesCoordinateStart + probesWorldDistance * float3(probe3DIndex);
}

float3 SampleProbeRadiance(uint probeIndex, float3 direction, Texture2D probeAtlasIrradiance, SamplerState sampler, uint2 probeResolution, uint3 probeGridDimentions)
{
    float2 probeUV = OctahedronEncode(direction);
    float4 uvRect = GetProbeAtlasUVRect(probeIndex, probeResolution, probeGridDimentions);

    float2 samplingUV = uvRect.xy + probeUV * (uvRect.zw - uvRect.xy);
    return probeAtlasIrradiance.SampleLevel(sampler, samplingUV, 0).rgb;
}

float SampleProbeVisibility(float testDistance, uint probeIndex, float3 direction, Texture2D probeAtlasVisibility, SamplerState sampler, uint2 probeResolution, uint3 probeGridDimentions)
{
    float2 probeUV = OctahedronEncode(direction);
    float4 uvRect = GetProbeAtlasUVRect(probeIndex, probeResolution, probeGridDimentions);

    float2 samplingUV = uvRect.xy + probeUV * (uvRect.zw - uvRect.xy);
    float2 visibilityMoments = probeAtlasVisibility.SampleLevel(sampler, samplingUV, 0).xy;

    // Chebyshev's inequality (aka Variance Shadow Map test)
    float variance = visibilityMoments.y - visibilityMoments.x * visibilityMoments.x;
    float diff = testDistance - visibilityMoments.x;
    float chebyshev = variance / (variance + diff * diff);

    return chebyshev;
}


struct DDGIProbeNeighborhood
{
    static const uint ProbeCount = 8;

    uint indices[ProbeCount];
    float weights[ProbeCount];
};

DDGIProbeNeighborhood GetProbeNeighborhood(float3 worldPosition, in DDGIConstantBuffer cb)
{
    float3 gridSpaceCoord = (worldPosition - cb.probeWorldOffset) / max(cb.probesWorldDistance.xyz, 0.001f) + float3(cb.probeGridDimentions) * 0.5f;
    uint3 baseProbeIndex = (uint3)floor(gridSpaceCoord);

    DDGIProbeNeighborhood neighborhood;
    for (int i = 0; i < DDGIProbeNeighborhood::ProbeCount; i++)
    {
        uint3 offset = uint3(i & 1, (i >> 1) & 1, (i >> 2) & 1);
        uint3 probeIndex = baseProbeIndex + offset;
        neighborhood.indices[i] = GetProbeIndex(cb.probeGridDimentions, probeIndex);
        neighborhood.weights[i] = (1.0f - abs(gridSpaceCoord.x - probeIndex.x)) *
                                  (1.0f - abs(gridSpaceCoord.y - probeIndex.y)) *
                                  (1.0f - abs(gridSpaceCoord.z - probeIndex.z));
    }

    return neighborhood;
}


#endif // !__cplusplus

#endif // _DDGI_COMMON_H_