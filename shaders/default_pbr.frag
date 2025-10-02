#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.h"
#include "common/lighting/lighting.h"
#include "common/forward.h"

struct PSInput
{
    float4                          ndc_pos      : SV_Position;
    [[vk::location(0)]] float3      wpos         : POSITION;
    [[vk::location(1)]] float2      uv           : TEXCOORD0;
    [[vk::location(2)]] float3      prev_wpos    : TEXCOORD1;
    [[vk::location(3)]] float3x3    TBN          : TEXCOORD2;
};

[require(spvRayQueryKHR)]
[shader("pixel")]
ForwardPassOutput main(PSInput input)
{
    S_INSTANCE* InstancePtr = GetInstance();
    S_MATERIAL* MaterialPtr = GetMaterial(InstancePtr);
    //uint instanceFlags = GetInstanceFlags(InstancePtr);
    uint materialFlags = GetMaterialFlags(MaterialPtr);

    float3 diffuse    = SampleGlobalTextureAnisotropic(GetDiffuseTextureID(MaterialPtr), input.uv).rgb;
    float3 normal     = SampleGlobalTextureAnisotropic(GetNormalTextureID(MaterialPtr), input.uv).rgb;
    float metalness   = 0.0;
    float roughness   = 0.0;

    if ((materialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_DEFAULT) != 0)
    {
        metalness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(MaterialPtr), input.uv).r;
        roughness = SampleGlobalTextureAnisotropic(GetRoughnessTextureID(MaterialPtr), input.uv).r;
    }
    if ((materialFlags & MATERIAL_FLAG_MATERIAL_TEXTURES_GLTF_SPHERES) != 0)
    {
        float2 metalness_roughness = SampleGlobalTextureAnisotropic(GetMetalnessTextureID(MaterialPtr), input.uv).bg;
        metalness = metalness_roughness.x;
        roughness = metalness_roughness.y;
    }

    float3 n = float3(0,0,1);
    if ((materialFlags & MATERIAL_FLAG_NORMAL_TEXTURE) != 0)
    {
        n = normalize(mul(input.TBN, (normal * 2.0 - 1.0)));
    }

    if ((materialFlags & MATERIAL_FLAG_NORMAL_TEXTURE_INVERT_Y) != 0)
    {
        normal = float3(normal.r, 1 - normal.g, normal.b);
        n = normalize(mul(input.TBN, (normal * 2.0 - 1.0)));
    }

    if ((materialFlags & MATERIAL_FLAG_NORMAL_TBN) != 0)
    {
        n = input.TBN[2];
    }

    S_SURFACE surface;
    surface.wpos = input.wpos;
    surface.normal = n;
    surface.diffuseSpectrum = diffuse;
    surface.roughness = roughness;
    surface.metalness = metalness;
    surface.prevWpos = float4(input.prev_wpos, 1.0);

    S_LIGHTING_RESULT lighting = CalculateLighting(surface);

    return OutputForwardPass(lighting, surface, input.ndc_pos);
}
