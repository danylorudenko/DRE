#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeData;
[[vk::binding(1, 3)]] StructuredBuffer<PrefixSums> groupPrefixSums;
[[vk::binding(2, 3)]] StructuredBuffer<PrefixSums> globalPrefixSums;
[[vk::binding(3, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeDataReordered;
[[vk::binding(4, 3)]] ConstantBuffer<DDGIConstantBuffer> ddgiCB;

/////////////////////////////////////////
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


/////////////////////////////////////////////////
groupshared uint s_CurrentBucketOffsets[DDGI_RADIX_SORT_BUCKET_COUNT];

groupshared uint4 s_PerWaveBucketCountsPacked[256 / 32];

groupshared uint s_IterationBucketCounts[DDGI_RADIX_SORT_BUCKET_COUNT];
groupshared uint s_IterationBucketStartOffsets[DDGI_RADIX_SORT_BUCKET_COUNT];

groupshared DDGIProbeEncodedData s_LocalSortedProbes[256];
groupshared uint s_LocalSortedProbesKeys[256];

/////////////////////////////////////////////////
[shader("compute")]
[numthreads(256, 1, 1)]
void reorderCS(uint groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    // fetch what are the offsets of the items inside their key-bucket for this Workgroup
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

        GroupMemoryBarrierWithGroupSync();

        uint4 waveBucketCounts = uint4(0,0,0,0);

        uint inputID = regionStart + i * 256 + GTid.x;
        bool isLaneValid = inputID < ddgiCB.totalProbeCount;

        DDGIProbeEncodedData probeEncodedData = (DDGIProbeEncodedData)0;
        uint key = 0;
        if (isLaneValid)
        {
            probeEncodedData = ddgiProbeData[inputID];
            DDGIProbeData probeData = DecodeProbeData(probeEncodedData);
            key = GetProbeSortingKey(probeData);
            key = (key >> (sortingPass * 4)) & 0xF;

            IncrementPackedKeyValue(waveBucketCounts, key);
            InterlockedAdd(s_IterationBucketCounts[key], 1);
        }

        // offsets for each item inside their bucket for this wave (rank inside wave)
        uint4 waveOffsetsInsideBuckets = WavePrefixSum(waveBucketCounts);
        waveBucketCounts = WaveActiveSum(waveBucketCounts);

        // save how many items in each bucket for this wave
        if (WaveIsFirstLane())
        {
            s_PerWaveBucketCountsPacked[waveId] = waveBucketCounts;
        }

        GroupMemoryBarrierWithGroupSync();

        // prefix-sum ranks of previous waves, so we get workgroup-level rank
        uint4 prevWavesOffsetsInsideBuckets = uint4(0,0,0,0);
        for (uint w = 0; w < waveId; w++)
        {
            prevWavesOffsetsInsideBuckets += s_PerWaveBucketCountsPacked[w];
        }

        // these are final group-level ranks for each item inside their bucket
        uint4 groupOffsetsInsideBuckets = prevWavesOffsetsInsideBuckets + waveOffsetsInsideBuckets;
        uint offsetInsideBucket = UnpackKeyValue(groupOffsetsInsideBuckets, key); // rank inside bucket

        // this is a group-level prefix sum (offset for each bucket start)
        if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
        {
            s_IterationBucketStartOffsets[GTid.x] = WavePrefixSum(s_IterationBucketCounts[GTid.x]);
        }

        GroupMemoryBarrierWithGroupSync();

        // group-level bucket start + rank inside bucket
        uint localSortedID = s_IterationBucketStartOffsets[key] + offsetInsideBucket;

        // do a "local sort" and cache what key was stored here
        if (isLaneValid)
        {
            s_LocalSortedProbes[localSortedID] = probeEncodedData;
            s_LocalSortedProbesKeys[localSortedID] = key; // this key is required so we later can lookup start of the bucket for this item
        }

        GroupMemoryBarrierWithGroupSync();

        if (isLaneValid)
        {
            uint outputKeyForThread = s_LocalSortedProbesKeys[GTid.x];
            uint localDistanceFromBucketStart = GTid.x - s_IterationBucketStartOffsets[outputKeyForThread];

            // global start of the bucket + group-level offset inside bucket + local rank of item inside bucket
            uint finalDestination = globalPrefixSums[0].sums[outputKeyForThread] + s_CurrentBucketOffsets[outputKeyForThread] + localDistanceFromBucketStart;
            ddgiProbeDataReordered[finalDestination] = s_LocalSortedProbes[GTid.x];
        }

        GroupMemoryBarrierWithGroupSync();

        // since we do multiple iterations in one workgroup, save work that we did in current iteration
        if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
        {
            s_CurrentBucketOffsets[GTid.x] += s_IterationBucketCounts[GTid.x];
        }
    }
}
