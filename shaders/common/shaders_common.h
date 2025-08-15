// mandatory shader include for all shaders in DRE
#ifndef _SHADERS_COMMON_H_
#define _SHADERS_COMMON_H_

#ifndef __cplusplus

#include "common/shaders_defines.h"

// Global push constants
[[vk::push_constant]]
cbuffer GlobalPushConstant
{
    uint value_int1;
} globalPushConstant;

// Global resources
[[vk::binding(0, 0)]]
SamplerState g_GlobalSamplers[4];

[[vk::binding(1, 0)]]
RaytracingAccelerationStructure g_TLAS;

[[vk::binding(0, 1)]]
Texture2D<float4> g_GlobalTextures[];

// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[id]

// Default samplers
#define GetSamplerNearest()      g_GlobalSamplers[0]
#define GetSamplerLinear()       g_GlobalSamplers[1]
#define GetSamplerLinearClamp()  g_GlobalSamplers[2]
#define GetSamplerAnisotropic()  g_GlobalSamplers[3]

// Texture sampling helpers
#define SampleTexture(texObj, sampObj, uv) texObj.Sample(sampObj, uv)
#define TexelFetchLvl(texObj, pos, lvl)    texObj.Load(int3(pos, lvl))
#define TexelFetch(texObj, pos)           texObj.Load(int3(pos, 0))

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

#endif // !__cplusplus

#endif // _SHADERS_COMMON_H_

