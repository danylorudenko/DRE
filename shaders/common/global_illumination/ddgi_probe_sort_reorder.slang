#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeData;
[[vk::binding(1, 3)]] StructuredBuffer<PrefixSums> groupPrefixSums;
[[vk::binding(2, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeDataReordered;


//groupshared uint s_LocalBucketCounts[DDGI_RADIX_SORT_BUCKET_COUNT];
//groupshared uint s_LocalPackedBucketCounts[DDGI_RADIX_SORT_BUCKET_COUNT / 4];
//groupshared uint s_LocalBucketPrefixSums[DDGI_RADIX_SORT_BUCKET_COUNT];
//groupshared uint s_LocalProbeOffsets[256];

//groupshared uint s_LocalKeyAccumulator[DDGI_RADIX_SORT_BUCKET_COUNT];

//groupshared uint4 itemOffsetTrackers;

uint UnpackKeyValue(in uint4 packedStorage, in uint key)
{
    uint keyValue = (packedStorage[key / 4] >> (8 * (key % 4))) & 0xFF;
    return keyValue;
}

void IncrementPackedKeyValue(inout uint4 packedStorage, in uint key, in uint increment = 1)
{
    uint packedIncrement = increment << (8 * (key % 4));
    packedStorage[key / 4] += packedIncrement;
}

void IncrementPackedKeyAtomic(inout uint4 packedStorage, in uint key, in uint increment = 1)
{
    uint packedIncrement = increment << (8 * (key % 4));
    InterlockedAdd(packedStorage[key / 4], packedIncrement);
}


uint4 PackKeyValues(in uint keyValues[DDGI_RADIX_SORT_BUCKET_COUNT])
{
    uint4 packedStorage = {};
    for (uint i = 0; i < DDGI_RADIX_SORT_BUCKET_COUNT; i++)
    {
        uint packedIncrement = keyValues[i] << (8 * (i % 4));
        packedStorage[i / 4] += packedIncrement;
    }

    return packedStorage;
}

void UnpackKeyValues(in uint4 packedKeyValues, inout uint keyValues[DDGI_RADIX_SORT_BUCKET_COUNT])
{
    for (uint i = 0; i < DDGI_RADIX_SORT_BUCKET_COUNT; i++)
    {
        keyValues[i] = UnpackKeyValue(packedKeyValues, i);
    }
}

groupshared uint s_CurrentBucketOffsets[DDGI_RADIX_SORT_BUCKET_COUNT];

groupshared uint4 s_PerWaveBucketCountsPacked[256 / 32];

groupshared uint s_IterationBucketCounts[DDGI_RADIX_SORT_BUCKET_COUNT];
groupshared uint s_IterationBucketStartOffsets[DDGI_RADIX_SORT_BUCKET_COUNT];

groupshared DDGIProbeEncodedData localSortedProbes[256];
groupshared uint localSortedProbesKeys[256];

[shader("compute")]
[numthreads(256, 1, 1)]
void reorderCS(uint groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
    {
        s_CurrentBucketOffsets[GTid.x] = groupPrefixSums[groupID].sums[GTid.x];
    }

    uint regionSize = globalPushConstant.value_uint.y; // ensure that it's multiple of 256 on CPU
    uint iterationsOverRegion = regionSize / 256;
    uint regionStart = groupID * regionSize;

    uint sortingPass = globalPushConstant.value_uint.x;

    uint waveId = GTid.x / WaveGetLaneCount();

    for (uint i = 0; i < iterationsOverRegion; i++)
    {
        if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
        {
            s_IterationBucketCounts[GTid.x] = 0;
        }

        uint4 waveBucketCounts;
        uint4 waveOffsetsInsideBuckets;

        uint inputID = regionStart + i * 256 + GTid.x;
        DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[inputID]);
        uint key =  GetProbeSortingKey(probeData);
        key = (key >> (sortingPass * 4)) & 0xF;

        IncrementPackedKeyValue(waveBucketCounts, key);
        InterlockedAdd(s_IterationBucketCounts[key], 1);

        waveOffsetsInsideBuckets = WavePrefixSum(waveBucketCounts);
        waveBucketCounts = WaveActiveSum(waveBucketCounts);

        if (WaveIsFirstLane())
        {
            s_PerWaveBucketCountsPacked[waveId] = waveBucketCounts;
        }

        GroupMemoryBarrierWithGroupSync();

        uint4 prevWavesOffsetsInsideBuckets = uint4(0,0,0,0);
        for (uint w = 0; w < waveId; w++)
        {
            prevWavesOffsetsInsideBuckets += s_PerWaveBucketCountsPacked[w];
        }

        uint4 groupOffsetsInsideBuckets = prevWavesOffsetsInsideBuckets + waveOffsetsInsideBuckets;
        uint offsetInsideBucket = UnpackKeyValue(groupOffsetsInsideBuckets, key); // rank inside bucket

        if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
        {
            s_IterationBucketStartOffsets[GTid.x] = WavePrefixSum(s_IterationBucketCounts[GTid.x]);
        }

        GroupMemoryBarrierWithGroupSync();


        uint localSortedID = s_IterationBucketStartOffsets[key] + offsetInsideBucket;

        localSortedProbes[localSortedID] = ddgiProbeData[inputID];
        localSortedProbesKeys[localSortedID] = key;

        GroupMemoryBarrierWithGroupSync();


        uint outputKeyForThread = localSortedProbesKeys[GTid.x];
        uint localDistanceFromBucketStart = GTid.x - s_IterationBucketStartOffsets[outputKeyForThread];

        uint finalDestination = s_CurrentBucketOffsets[outputKeyForThread] + localDistanceFromBucketStart;
        ddgiProbeDataReordered[finalDestination] = localSortedProbes[GTid.x];

        if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
        {
            s_CurrentBucketOffsets[GTid.x] += s_IterationBucketCounts[GTid.x];
        }
    }
}
