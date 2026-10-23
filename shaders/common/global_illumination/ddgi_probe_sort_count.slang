#include "shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] RWStructuredBuffer<DDGIProbeEncodedData> ddgiProbeData;
[[vk::binding(1, 3)]] RWStructuredBuffer<BucketCounts> bucketCounts;

groupshared uint s_Buckets[DDGI_RADIX_SORT_BUCKET_COUNT];

[shader("compute")]
[numthreads(256, 1, 1)]
void countCS(uint groupID : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
    {
        s_Buckets[GTid.x] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint regionSize = globalPushConstant.value_uint.y; // ensure that it's multiple of 256 on CPU
    uint iterationsOverRegion = regionSize / 256;
    uint regionStart = groupID * regionSize;
    
    for (uint i = 0; i < iterationsOverRegion; i++)
    {
        uint inputID = regionStart + i * 256 + GTid.x;
        DDGIProbeData probeData = DecodeProbeData(ddgiProbeData[inputID]);
        uint key = GetProbeSortingKey(probeData);

        uint sortingPass = globalPushConstant.value_uint.x;
        key = (key >> (sortingPass * 4)) & 0xF;

        InterlockedAdd(s_Buckets[key], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    if (GTid.x < DDGI_RADIX_SORT_BUCKET_COUNT)
    {
        InterlockedAdd(bucketCounts[groupID].buckets[GTid.x], s_Buckets[GTid.x]);
    }
}
