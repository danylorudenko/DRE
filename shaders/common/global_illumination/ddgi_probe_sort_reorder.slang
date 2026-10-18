#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"


struct BucketCounts
{
    uint buckets[DDGI_RADIX_SORT_BUCKET_COUNT];
}

struct PrefixSums
{
    uint sums[DDGI_RADIX_SORT_BUCKET_COUNT + 1];
}


[[vk::binding(0, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeData;
[[vk::binding(1, 3)]] StructuredBuffer<BucketCounts> groupBucketCounts;
[[vk::binding(2, 3)]] StructuredBuffer<PrefixSums> groupPrefixSums;
[[vk::binding(3, 3)]] StructuredBuffer<PrefixSums> globalPrefixSum;

[[vk::binding(4, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeDataReordered;


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


[shader("compute")]
[numthreads(32, 1, 1)]
void reorderCS(uint groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    static const uint regionSize = 256;
    static const uint iterationsOverRegion = regionSize / 32;
    uint regionStart = groupID * regionSize;

    uint sortingPass = globalPushConstant.value_uint.x;

    uint4 totalBucketCountsPacked = {};
    uint totalBucketCounts[DDGI_RADIX_SORT_BUCKET_COUNT] = {};
    uint totalBucketPrefixSums[DDGI_RADIX_SORT_BUCKET_COUNT] = {};

    uint4 localBucketCounts[iterationsOverRegion] = {};
    uint4 localBucketPrefixSums[iterationsOverRegion] = {};
    uint keysForThread[iterationsOverRegion];

    for (uint i = 0; i < iterationsOverRegion; i++)
    {
        uint inputID = regionStart + i * 32 + GTid.x;
        DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[inputID]);
        uint key =  GetProbeSortingKey(probeData);
        key = (key >> (sortingPass * 4)) & 0xF;
        keysForThread[i] = key;

        // each thread increments its own bucket count (packed)
        IncrementPackedKeyValue(localBucketCounts[i], key);

        // this will be a position of this input item inside it's bucket
        localBucketPrefixSums[i] = WavePrefixSum(localBucketCounts[i]);

        // then threads share-sum bucket counts which results basically in step 1 of the radix sort
        localBucketCounts[i] = WaveActiveSum(localBucketCounts[i]);

        // because we do multiple iterations over this, accumulate totals in outer scope
        totalBucketCountsPacked += localBucketCounts[i];
    }

    bool specialCaseDetected = false;
    uint specialCaseBucket = 0;
    for (uint i = 0; i < DDGI_RADIX_SORT_BUCKET_COUNT; i++)
    {
        totalBucketCounts[i] = UnpackKeyValue(totalBucketCountsPacked, i);
        uint testCount = groupBucketCounts[groupID].buckets[i];
        if (testCount == regionSize)
        {
            specialCaseDetected = true;
            specialCaseBucket = i;
            break;
        }
    }

    // all 256 elements are of the same key, no local sort required
    if (specialCaseDetected)
    {
        // all items in this region belong to the same bucket, so we can just copy them over
        for (uint i = 0; i < iterationsOverRegion; i++)
        {
            uint inputID = regionStart + i * 32 + GTid.x;

            DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[inputID]);
            uint key =  GetProbeSortingKey(probeData);
            key = (key >> (sortingPass * 4)) & 0xF;

            uint globalKeyOffset = globalPrefixSum[0].sums[key];
            uint workgroupKeyOffset = groupPrefixSums[groupID].sums[key];
            uint globalSortPosition = globalKeyOffset + workgroupKeyOffset + inputID;
            ddgiProbeDataReordered[globalSortPosition] = ddgiProbeData[inputID];
        }
        return;
    }

    for (uint i = 1; i < DDGI_RADIX_SORT_BUCKET_COUNT; i++)
    {
        totalBucketPrefixSums[i] = totalBucketPrefixSums[i - 1] + totalBucketCounts[i - 1];
    }

    uint localKeyOffsetTrackers[DDGI_RADIX_SORT_BUCKET_COUNT] = {};
    for (uint i = 0; i < iterationsOverRegion; i++)
    {
        uint key = keysForThread[i];

        uint keyCount = totalBucketCounts[key];

        uint key32PositionInBucket = UnpackKeyValue(localBucketPrefixSums[i], key);
        uint itemPositionInBucket = localKeyOffsetTrackers[key] + key32PositionInBucket;
        uint localSortPosition = totalBucketPrefixSums[key] + itemPositionInBucket;

        uint globalKeyOffset = globalPrefixSum[0].sums[key];
        uint workgroupKeyOffset = groupPrefixSums[groupID].sums[key];
        uint globalSortPosition = globalKeyOffset + workgroupKeyOffset + itemPositionInBucket;

        ddgiProbeDataReordered[globalSortPosition] = ddgiProbeData[regionStart + i * 32 + GTid.x];

        for (uint j = 0; j < DDGI_RADIX_SORT_BUCKET_COUNT; j++)
        {
            localKeyOffsetTrackers[j] += UnpackKeyValue(localBucketCounts[i], j);
        }
    }

    
}
