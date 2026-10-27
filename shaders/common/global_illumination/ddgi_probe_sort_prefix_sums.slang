#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<BucketCounts> groupBucketCount;
[[vk::binding(1, 3)]] RWStructuredBuffer<PrefixSums> groupPrefixSums;
[[vk::binding(2, 3)]] RWStructuredBuffer<PrefixSums> globalPrefixSum;
[[vk::binding(3, 3)]] ConstantBuffer<DDGIConstantBuffer> ddgiCB;

[shader("compute")]
[numthreads(DDGI_RADIX_SORT_BUCKET_COUNT, 1, 1)]
void prefixSumsCS(uint3 DTid : SV_DispatchThreadID)
{
    uint regionSize = globalPushConstant.value_uint.y; // ensure that it's multiple of 256 on CPU
    uint workGroupCount = (ddgiCB.totalProbeCount + regionSize - 1) / regionSize;

    // just filling first 16 elements of the prefix sum array, as we have only 16 buckets
    groupPrefixSums[0].sums[DTid.x] = 0;

    uint sum = 0;
    for (uint i = 1; i < workGroupCount; ++i)
    {
        sum += groupBucketCount[i - 1].buckets[DTid.x];
        groupPrefixSums[i].sums[DTid.x] = sum;
    }

    sum += groupBucketCount[workGroupCount - 1].buckets[DTid.x];
    uint keyGlobalOffset = WavePrefixSum(sum);

    globalPrefixSum[0].sums[DTid.x] = keyGlobalOffset;
}
