#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.slang"
#include "common/lighting/lighting.slang"
#include "common/forward.slang"

#include "common/vertex/dre_vertex_layout.slang"

struct PSInput
{
    float4                          ndc_pos      : SV_Position;
    [[vk::location(0)]] float3      wpos         : POSITION;
    [[vk::location(1)]] float2      uv           : TEXCOORD0;
    [[vk::location(2)]] float3      prev_wpos    : TEXCOORD1;
    [[vk::location(3)]] float3x3    TBN          : TEXCOORD2;
};

[shader("vertex")]
PSInput mainVS(VSInput input)
{
    S_INSTANCE* InstancePtr = GetInstance();

    PSInput output;
    float4x4 model_mat = GetWorldTransform(InstancePtr);
    output.wpos = mul(model_mat, float4(input.pos, 1)).xyz;
    output.uv = input.uv;

    float4 ndc_pos = mul(GetCameraViewProjM(), float4(output.wpos, 1.0));
    ndc_pos.xy += (GetJitter() * ndc_pos.w); // perspective-correct jitter

    output.ndc_pos = ndc_pos;
    output.prev_wpos = mul(GetInvWorldTransform(InstancePtr), float4(input.pos, 1.0)).xyz;

    float3 T = normalize(mul(model_mat, float4(input.tan.xyz, 0.0)).xyz);
    float3 B = normalize(mul(model_mat, float4(input.btan, 0.0)).xyz);
    float3 N = normalize(mul(model_mat, float4(input.norm, 0.0)).xyz);

    output.TBN = float3x3(T, B, N);

    return output;
}


[require(spvRayQueryKHR)]
[shader("pixel")]
ForwardPassOutput mainPS(PSInput input)
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
