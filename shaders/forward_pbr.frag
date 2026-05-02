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
    
    S_MATERIAL_PROPERTIES MaterialProperties = ReadMaterialProperties(MaterialPtr, input.uv, input.TBN);

    S_SURFACE surface;
    surface.wpos = input.wpos;
    surface.normal = MaterialProperties.normal;
    surface.diffuseSpectrum = MaterialProperties.diffuse;
    surface.roughness = MaterialProperties.roughness;
    surface.metalness = MaterialProperties.metalness;
    surface.prevWpos = input.prev_wpos;

    S_LIGHTING_RESULT lighting = CalculateLighting(surface, ShaderStage::Pixel);

    return OutputForwardPass(lighting, surface, input.ndc_pos);
}
