#define ENABLE_PCF
#define ENABLE_PCF_POISSON

#include "common/shaders_common.slang"
#include "common/lighting/lighting.slang"
#include "common/vertex/dre_vertex_layout.slang"
#include "common/gbuffer.slang"


struct PSInput
{
    float4                          ndc_pos      : SV_Position;
    [[vk::location(0)]] float3      wpos         : POSITION;
    [[vk::location(1)]] float2      uv           : TEXCOORD0;
    [[vk::location(2)]] float3      prev_wpos    : TEXCOORD1;
    [[vk::location(3)]] float3x3    TBN          : TEXCOORD2;
};

[shader("vertex")]
PSInput mainVS(DREVertex input)
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

    #if 0
    input.tan = cross(input.norm, input.btan);
    if (all(input.btan < 0.0))
    {
        input.tan = -input.tan;
    }
    #endif

    #if 0
    input.btan = cross(input.norm, input.tan);
    if (all(input.tan < 0.0))
    {
        input.btan = -input.btan;
    }
    #endif

    #if 0
    input.tan = input.tan - dot(input.tan, input.norm) * input.norm;
    input.btan = cross(input.norm, input.tan);
    #endif

    #if 0
    input.btan = input.btan - dot(input.btan, input.norm) * input.norm;
    input.tan = cross(input.norm, input.btan);
    #endif

    float3 T = normalize(mul(model_mat, float4(input.tan.xyz, 0.0)).xyz);
    float3 B = normalize(mul(model_mat, float4(input.btan, 0.0)).xyz);
    float3 N = normalize(mul(model_mat, float4(input.norm, 0.0)).xyz);

    output.TBN = float3x3(T, B, N);

    return output;
}


[require(spvRayQueryKHR)]
[shader("pixel")]
GBuffer mainPS(PSInput input)
{
    S_INSTANCE* InstancePtr = GetInstance();
    S_MATERIAL* MaterialPtr = GetMaterial(InstancePtr);

    S_MATERIAL_PROPERTIES MaterialProperties = ReadMaterialProperties(MaterialPtr, input.uv, input.TBN, MaterialTextureSampler());

    if (MaterialProperties.opacity < 1.0f)
    {
        discard;
    }

    GBuffer Output;
    Output.Diffuse_Roughness = float4(MaterialProperties.diffuse, MaterialProperties.roughness);
    float3 encodedNormal = MaterialProperties.normal * 0.5f + 0.5f;
    Output.Normal_Metalness = float4(encodedNormal, MaterialProperties.metalness);
    Output.Velocity = CalculateVelocity(input.ndc_pos, input.prev_wpos);
    Output.ObjectID = SceneNodeID2Color();

    //Output.DEBUG_TEXTURE = float4(input.TBN[2], 1);
    //Output.DEBUG_TEXTURE = float4(frac(input.uv), 0, 1);
    Output.DEBUG_TEXTURE = float4(MaterialProperties.opacity.rrr, 1);
    
    return Output;
}
