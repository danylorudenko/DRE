// mandatory shader include for all shaders in DRE
#ifndef _SHADERS_COMMON_H_
#define _SHADERS_COMMON_H_

#ifndef __cplusplus

#include "common/shaders_defines.h"
#include "common/global_uniform.h"

/////////////////////////////
// Global push constants
struct GlobalPushConstant
{
    uint value_int1;
};
[[vk::push_constant]] GlobalPushConstant globalPushConstant;

//////////////////////////////
// Global resources
[[vk::binding(0, 0)]] SamplerState g_GlobalSamplers[];
[[vk::binding(1, 0)]] RaytracingAccelerationStructure g_TLAS;

[[vk::binding(0, 1)]] Texture2D<float4> g_GlobalTextures[];

// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[id]

// Default samplers
#define GetSamplerNearest()      g_GlobalSamplers[0]
#define GetSamplerLinear()       g_GlobalSamplers[1]
#define GetSamplerLinearClamp()  g_GlobalSamplers[2]
#define GetSamplerAnisotropic()  g_GlobalSamplers[3]

// Texture sampling helpers
//#define SampleTexture(texObj, sampObj, uv)  texObj.Sample(sampObj, uv)
//#define TexelFetchLvl(texObj, pos, lvl)     texObj.Load(int3(pos, lvl))
//#define TexelFetch(texObj, pos)             texObj.Load(int3(pos, 0))

float4 ReadGlobalTexture(uint id, int2 coords)
{
    return g_GlobalTextures[id][coords];
}

float4 SampleGlobalTextureLinear(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerLinear(), uv);
}

float4 SampleGlobalTextureAnisotropic(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerAnisotropic(), uv);
}

float sRGB2Linear(float x)
{
    return pow(x, 1.0 / 2.2);
}

float3 sRGB2Linear(float3 x)
{
    return pow(x, 1.0 / 2.2);
}

float Linear2sRGB(float x)
{
    return pow(x, 2.2);
}

float3 Linear2sRGB(float3 x)
{
    return pow(x, 2.2);
}

float ConvertDeviceZToViewZ(float deviceZ)
{
    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];

    return B / (deviceZ - A);
}

float2 ConvertScreenToUV(int2 pixel)
{
    return (float2(pixel) + 0.5) / GetViewportSize();
}

float2 ConvertScreenToNDC(int2 pixel)
{
    return ((float2(pixel) + 0.5) / GetViewportSize()) * 2 - 1;
}

float3 ConvertScreenToWorld(int2 pixel, float deviceZ)
{
    float zView = ConvertDeviceZToViewZ(deviceZ);

    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];
    float zClip = A * zView + B;

    float2 pixelNDC = ((float2(pixel) + 0.5) / GetViewportSize()) * 2 - 1;
    float4 pixelClip = float4(float2(pixelNDC) * zView, zClip, zView);

    float4 pixelWorldHomogeneous = mul(GetCameraiViewProjM(), pixelClip);
    return pixelWorldHomogeneous.xyz / pixelWorldHomogeneous.w;
}

float2 CalculateVelocity(float4 currPosClip, float3 prevWorldPos)
{
    float4 prevNDC = mul(GetPrevCameraViewProjM(), float4(prevWorldPos, 1.0));
    prevNDC.xy /= prevNDC.w;

    float2 currNDC = currPosClip.xy / currPosClip.w;

    float2 vel = (currNDC - prevNDC.xy);
    return vel * 0.5f;
}

float2 RadialKernel(float2 center, float angleRadians, float2 extent)
{
    float2 xy;
    sincos(angleRadians, xy[0], xy[1]);

    return xy * extent + center;
}

#endif // !__cplusplus

#endif // _SHADERS_COMMON_H_

