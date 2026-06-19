#include "common/shaders_common.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] ConstantBuffer<DDGIConstantBuffer> ddgiCB;
[[vk::binding(1, 3)]] Texture2D ddgiGBufferA;
[[vk::binding(2, 3)]] Texture2D ddgiGBufferB;
[[vk::binding(3, 3)]] RWTexture2D ddgiAtlasIrradiance;
[[vk::binding(4, 3)]] StructuredBuffer<DDGIProbeData> ddgiProbeData;
// [[vk::binding(3, 3)]] Texture2D ddgiAtlasIrradiancePrev;

[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    float4 gbufferA = ddgiGBufferA[DTid.xy];
    float4 gbufferB = ddgiGBufferB[DTid.xy];

    uint probeIndex = GetProbeIndexFromPixel(DTid.xy, ddgiCB.probeGBufferResolution, ddgiCB.probeGridDimentions);

    S_SURFACE surface = {};
    surface.diffuseSpectrum = gbufferA.rgb;
    surface.roughness = gbufferA.a;
    surface.normal = gbufferB.xyz * 2.0 - 1.0;
    surface.metalness = gbufferB.a;
    surface.ao = 1;

    DDGIProbeData probeData = ddgiProbeData[probeIndex];
    surface.wpos = probeData.position.xyz;
    surface.prevWpos = probeData.position.xyz;

    S_LIGHTING_RESULT lightingResult = CalculateDirectLighting(surface, ShaderStage::Compute);

    float3 irradiance = lightingResult.finalRadiance * ddgiCB.irradianceUpdateRate + ddgiAtlasIrradiance[DTid.xy].rgb * (1.0 - ddgiCB.irradianceUpdateRate);
    ddgiAtlasIrradiance[DTid.xy] = float4(irradiance, 1.0);

}