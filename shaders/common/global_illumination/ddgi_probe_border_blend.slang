#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer>    ddgiCB;


[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
}