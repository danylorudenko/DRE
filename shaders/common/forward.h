#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "common/shaders_defines.h"
#include "common/global_uniform.h"

//////////////
#ifndef __cplusplus
[[vk::binding(0, 3)]] Texture2D<float> shadowMap;
#endif

//////////////
struct ForwardUniform
{
    float4x4  shadow_VP;
    float4    shadow_size;
};
DeclareConstantBuffer(ForwardUniform, passUniform, 3, 1)

//////////////
#ifndef __cplusplus
[[vk::binding(2, 3)]] Texture2D<float4> causticMap;
#endif

///////////////////////////////////////
// ====================================
///////////////////////////////////////

/////////////
// Object ID
#ifndef __cplusplus
float4 GlobalID2Color()
{
    S_INSTANCE_GPURef Instance = GetInstance();

    float r = (GetGlobalID(Instance) & 0xFF000000) >> 24;
    float g = (GetGlobalID(Instance) & 0x00FF0000) >> 16;
    float b = (GetGlobalID(Instance) & 0x0000FF00) >> 8;
    float a = (GetGlobalID(Instance) & 0x000000FF) >> 0;
    return float4(r,g,b,a) / 255.0f;
}
#endif // __cplusplus

/////////////
// Output
#ifdef DRE_FRAGMENT_SHADER
struct ForwardPassOutput
{
    float4 finalColor : SV_Target0;
    float2 velocity   : SV_Target1;
    float4 id         : SV_Target2;
};
#endif // DRE_FRAGMENT_SHADER
#define FORWARD_PASS_OUTPUT_COUNT 3 // DON'T FORGET

/////////////
// Reusable outputs
#ifdef DRE_FRAGMENT_SHADER
void OutputForwardPass(in S_LIGHTING_RESULT Result, in S_SURFACE Surface, float4 fragCoord, out ForwardPassOutput outp)
{
    outp.finalColor = float4(Result.finalRadiance, 1.0f);

    float4 prev_ndc = mul(GetPrevCameraViewProjM(), Surface.prevWpos);
    prev_ndc /= prev_ndc.w;

    float2 pixel_pos_uv = fragCoord.xy / GetViewportSize();
    float2 pixel_pos_ndc = pixel_pos_uv * 2.0f - 1.0f;

    float2 vel = (pixel_pos_ndc - prev_ndc.xy);
    float2 vel_uv = vel * 0.5f;

    outp.velocity = vel_uv;
    outp.id = GlobalID2Color();
}
#endif // DRE_FRAGMENT_SHADER

#endif // __FORWARD_H__
