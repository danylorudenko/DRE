#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.h"
#include "common/lighting/lighting.h"

struct PSInput
{
    float4                          ndc_pos      : SV_Position;
    [[vk::location(0)]] float3      wpos         : POSITION;
    [[vk::location(1)]] float2      uv           : TEXCOORD0;
    [[vk::location(2)]] float3      prev_wpos    : TEXCOORD1;
    [[vk::location(3)]] float3x3    TBN          : TEXCOORD2;
};

struct GBuffer
{
    float4 Diffuse_Roughness    : SV_Target0;
    float4 Normal_Metalness     : SV_Target1;
    float2 Velocity             : SV_Target2;
    float4 ObjectID             : SV_Target3;
};

[require(spvRayQueryKHR)]
[shader("pixel")]
GBuffer main(PSInput input)
{
    S_INSTANCE* InstancePtr = GetInstance();
    S_MATERIAL* MaterialPtr = GetMaterial(InstancePtr);
    
    S_MATERIAL_PROPERTIES MaterialProperties = ReadMaterialProeprties(MaterialPtr, input.uv, input.TBN);

    GBuffer Output;
    Output.Diffuse_Roughness = float4(MaterialProperties.diffuse, MaterialProperties.roughness);
    Output.Normal_Metalness = float4(MaterialProperties.normal, MaterialProperties.metalness);
    Output.Velocity = CalculateVelocity(input.ndc_pos, input.prev_wpos);
    Output.ObjectID = GlobalID2Color();

    return Output;
}
