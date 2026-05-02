#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;


[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 1. read some noise
    // 2. based on noise generate random direction
    // 3. trace ray from the probe location towards the chosen direction
    // 4. based on ray direction:
    //   4.1 calculate topleft probe pixel
    //   4.2 calculate UV inside probe patch
    //   4.3 based on UV get weights for each pixel
    // 5. write weighted ray hit result to probe pixels
}