#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.slang"
#include "common/lighting/lighting.slang"

[[vk::binding(0, 3)]] RWTexture2D<float4>   lightingOutput;
[[vk::binding(1, 3)]] Texture2D<float4>     gBufferDiffuseRoughness;
[[vk::binding(2, 3)]] Texture2D<float4>     gBufferNormalMetalness;
[[vk::binding(3, 3)]] Texture2D<float>      sceneDepth;
[[vk::binding(4, 3)]] Texture2D<float>      ambientOcclusion;


[require(spvRayQueryKHR)]
[shader("compute")]
[numthreads(8, 8, 1)]
void mainCS(uint3 DTid : SV_DispatchThreadID)
{
    uint2 viewport = uint2(uint(GetViewportSize().x), uint(GetViewportSize().y));
    if (DTid.x >= viewport.x || DTid.y >= viewport.y)
    {
        return;
    }

    uint2 pixel = uint2(DTid.xy);
    float depth = sceneDepth[pixel];
    if (depth >= 1.0f)
    {
        lightingOutput[pixel] = float4(0.9f, 0.9f, 0.9f, 0.0f);
        return;
    }

    float4 gDiffuse = gBufferDiffuseRoughness[pixel];
    float4 gNormal = gBufferNormalMetalness[pixel];
    float ao = ambientOcclusion[pixel];

    float3 wPos = ConvertScreenToWorld(pixel, depth);
    S_SURFACE surface = InitSurface(wPos);

    surface.normal = normalize(gNormal.xyz * 2.0f - 1.0f);
    surface.diffuseSpectrum = gDiffuse.rgb;
    surface.roughness = gDiffuse.a;
    surface.metalness = gNormal.w;
    surface.ao = ao;
    surface.prevWpos = surface.wpos;

    bool drawProbeCage = all(GetCursorPos() == pixel) && (GetDDGI().drawSelectedCage != 0);

    S_LIGHTING_RESULT directLighting = CalculateDirectLighting(surface, GetCameraPos(), ShaderStage::Compute);
    S_LIGHTING_RESULT indirectLighting = CalculateIndirectLighting(surface, GetDDGI().irradianceAtlasTextureID, GetDDGI().visibilityAtlasTextureID, drawProbeCage);

    lightingOutput[pixel] = float4(directLighting.finalRadiance + indirectLighting.finalRadiance, 1.0f);
}
