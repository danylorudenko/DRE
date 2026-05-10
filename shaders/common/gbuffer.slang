#ifndef _GBUFFER_SLANG_
#define _GBUFFER_SLANG_

struct GBuffer
{
    float4 Diffuse_Roughness    : SV_Target0;
    float4 Normal_Metalness     : SV_Target1;
    float2 Velocity             : SV_Target2;
    float4 ObjectID             : SV_Target3;

    float4 DEBUG_TEXTURE        : SV_Target4;
};

#endif // _GBUFFER_SLANG_