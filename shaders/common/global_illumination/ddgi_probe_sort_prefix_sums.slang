#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

struct BucketCounts
{
    uint buckets[DDGI_RADIX_SORT_BUCKET_COUNT];
}

struct PrefixSums
{
    uint sums[DDGI_RADIX_SORT_BUCKET_COUNT];
}

[[vk::binding(0, 3)]] RWStructuredBuffer<BucketCounts> groupBucketCount;
[[vk::binding(1, 3)]] RWStructuredBuffer<PrefixSums> groupPrefixSums;
[[vk::binding(2, 3)]] RWStructuredBuffer<PrefixSums> globalPrefixSum;

groupshared PrefixSums s_GlobalPrefixSum;

[shader("compute")]
[numthreads(256, 1, 1)]
void prefixSumsCS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
    {
        s_GlobalPrefixSum.sums[DTid.x] = 0;
    }

    PrefixSums prefixSum;
    prefixSum.sums[0] = 0;
    for (uint i = 1; i < DDGI_RADIX_SORT_BUCKET_COUNT; i++)
    {
        prefixSum.sums[i] = prefixSum.sums[i - 1] + groupBucketCount[DTid.x].buckets[i - 1];
        InterlockedAdd(s_GlobalPrefixSum.sums[i], prefixSum.sums[i]);
    }

    groupPrefixSums[DTid.x] = prefixSum;

    GroupMemoryBarrierWithGroupSync();

    if (DTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
    {
        globalPrefixSum[0].sums[DTid.x] = s_GlobalPrefixSum.sums[DTid.x];
    }
}
