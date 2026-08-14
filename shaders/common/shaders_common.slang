// mandatory shader include for all shaders in DRE
#ifndef _SHADERS_COMMON_H_
#define _SHADERS_COMMON_H_

#include "common/shaders_defines.slang"

#ifndef __cplusplus
#include "common/global_uniform.slang"
#include "common/debug_draw.slang"


/////////////////////////////
// ShaderStage enum for compile-time branching
enum ShaderStage
{
    Vertex,
    Pixel,
    Compute
};


/////////////////////////////
// Global push constants
struct GlobalPushConstant
{
    uint value_int1;
};
[[vk::push_constant]] GlobalPushConstant globalPushConstant;
/////////////////////////////


/////////////////////////////
// Global resources

// set 0
[[vk::binding(0, 0)]] SamplerState g_GlobalSamplers[];
// set 1
[[vk::binding(0, 1)]] Texture2D<float4> g_GlobalTextures[];

// set 2
//[[vk::binding(0, 2)]] ConstantBuffer<GlobalUniforms> g_GlobalUniforms; // comes from global_uniform.slang
[[vk::binding(1, 2)]] RaytracingAccelerationStructure g_TLAS;
/////////////////////////////


// Global textures
#define GetGlobalTexture(id) g_GlobalTextures[id]

// Default samplers
#define GetSamplerNearest()      g_GlobalSamplers[0]
#define GetSamplerNearestClamp() g_GlobalSamplers[1]
#define GetSamplerLinear()       g_GlobalSamplers[2]
#define GetSamplerLinearClamp()  g_GlobalSamplers[3]
#define GetSamplerAnisotropic()  g_GlobalSamplers[4]

// Texture sampling helpers
//#define SampleTexture(texObj, sampObj, uv)  texObj.Sample(sampObj, uv)
//#define TexelFetchLvl(texObj, pos, lvl)     texObj.Load(int3(pos, lvl))
//#define TexelFetch(texObj, pos)             texObj.Load(int3(pos, 0))

float4 ReadGlobalTexture(uint id, int2 coords)
{
    return g_GlobalTextures[id][coords];
}

float4 SampleGlobalTextureNearest(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerNearest(), uv);
}

float4 SampleGlobalTextureNearest(uint id, float2 uv, float mipLevel)
{
    return g_GlobalTextures[id].SampleLevel(GetSamplerNearest(), uv, mipLevel);
}

float4 SampleGlobalTextureNearestClamp(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerNearestClamp(), uv);
}

float4 SampleGlobalTextureNearestClamp(uint id, float2 uv, float mipLevel)
{
    return g_GlobalTextures[id].SampleLevel(GetSamplerNearestClamp(), uv, mipLevel);
}

float4 SampleGlobalTextureLinear(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerLinear(), uv);
}

float4 SampleGlobalTextureLinear(uint id, float2 uv, float mipLevel)
{
    return g_GlobalTextures[id].SampleLevel(GetSamplerLinear(), uv, mipLevel);
}

float4 SampleGlobalTextureLinearClamp(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerLinearClamp(), uv);
}

float4 SampleGlobalTextureAnisotropic(uint id, float2 uv)
{
    return g_GlobalTextures[id].Sample(GetSamplerAnisotropic(), uv);
}

float4 SampleGlobalTextureAnisotropic(uint id, float2 uv, float mipLevel)
{
    return g_GlobalTextures[id].SampleLevel(GetSamplerAnisotropic(), uv, mipLevel);
}

Texture2D<float4> GetGlobalTextureObject(uint id)
{
    return g_GlobalTextures[id];
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

float2 ConvertViewOffsetToScreenOffset(float2 viewOffset, float zViewDistance)
{
    float _00 = GetCameraProjM()[0][0];
    float _11 = GetCameraProjM()[1][1];

    // -z because in right handed z is towards the camera (everything in VS has negative Z)
    float viewZ = max(0.0001, -zViewDistance);

    float2 NDC = (viewOffset.xy * float2(_00, _11)) / viewZ;
    return NDC * GetViewportSize() * 0.5;
}

float3 ConvertScreenToView(int2 pixel, float deviceZ)
{
    float zView = ConvertDeviceZToViewZ(deviceZ);

    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];
    float zClip = A * zView + B;

    float2 pixelNDC = ((float2(pixel) + 0.5) / GetViewportSize()) * 2 - 1;
    float4 pixelClip = float4(pixelNDC * zView, zClip, zView);

    float4 pixelViewHomogeneous = mul(GetCameraInvProjM(), pixelClip);
    return pixelViewHomogeneous.xyz / pixelViewHomogeneous.w;
}

float3 ConvertScreenToWorld(int2 pixel, float deviceZ)
{
    float zView = ConvertDeviceZToViewZ(deviceZ);

    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];
    float zClip = A * zView + B;

    float2 pixelNDC = ((float2(pixel) + 0.5) / GetViewportSize()) * 2 - 1;
    float4 pixelClip = float4(pixelNDC * zView, zClip, zView);

    float4 pixelWorldHomogeneous = mul(GetCameraInvViewProjM(), pixelClip);
    return pixelWorldHomogeneous.xyz / pixelWorldHomogeneous.w;
}

float3 ConvertUVzViewToWorld(float2 uv, float zView)
{
    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];
    float zClip = A * zView + B;

    float2 pixelNDC = uv * 2 - 1;
    float4 pixelClip = float4(pixelNDC * zView, zClip, zView);

    float4 pixelWorldHomogeneous = mul(GetCameraInvViewProjM(), pixelClip);
    return pixelWorldHomogeneous.xyz / pixelWorldHomogeneous.w;
}

float3 ConvertUVzViewToView(float2 uv, float zView)
{
    float A = GetCameraProjM()[2][2];
    float B = GetCameraProjM()[3][2];
    float zClip = A * zView + B;

    float2 pixelNDC = uv * 2 - 1;
    float4 pixelClip = float4(pixelNDC * zView, zClip, zView);

    float4 pixelWorldHomogeneous = mul(GetCameraInvProjM(), pixelClip);
    return pixelWorldHomogeneous.xyz / pixelWorldHomogeneous.w;
}

float3 ConvertUVToWorld(float2 uv, float deviceZ)
{
    float zView = ConvertDeviceZToViewZ(deviceZ);
    return ConvertUVzViewToWorld(uv, zView);
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

float3 PolarToCartesian(float elevationAngle, float azimuthAngle)
{
    float4 paSinCos;
    sincos(elevationAngle, paSinCos[0], paSinCos[1]);
    sincos(azimuthAngle, paSinCos[2], paSinCos[3]);

    float x = paSinCos[0] * paSinCos[3];
    float y = paSinCos[1];
    float z = paSinCos[0] * paSinCos[2];

    return float3(x, y, z);
}

float2 CartesianToPolar(float3 cartesian)
{
    float elevation = asin(cartesian.y);
    float azimuth = atan2(cartesian.z, cartesian.x);

    return float2(elevation, azimuth);
}

uint PcgHash(inout uint seedState) // https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
{
    uint state = seedState;
    seedState = seedState * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint2 PcgHash2D(uint2 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    return v;
}

//////////////////////////////////
// Debug Draw
void DrawDebugLine(float3 pos0, float3 pos1, float3 color)
{
    if (g_GlobalUniforms.debugDrawEnabled == 0)
        return;

    uint commandIndex;
    InterlockedAdd(g_GlobalUniforms.DebugDrawBuffer->commandCount, 1, commandIndex);

    if (commandIndex >= DEBUG_DRAW_COMMAND_CAPACITY)
        return;

    S_DEBUG_DRAW_COMMAND cmd;
    cmd.type = DebugDrawCommandType::Line;
    cmd.flags = 0;
    cmd.colorRGBA8 = pack_clamp_u8(uint4(float4(color, 1.0) * 255.0));

    cmd.p0 = float4(pos0, 1.0);
    cmd.p1 = float4(pos1, 1.0);

    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex] = cmd;
}

void DrawDebugCrossAligned(float3 pos0, float size, float3 color)
{
    if (g_GlobalUniforms.debugDrawEnabled == 0)
        return;

    uint commandIndex;
    InterlockedAdd(g_GlobalUniforms.DebugDrawBuffer->commandCount, 3, commandIndex);

    if (commandIndex + 3 >= DEBUG_DRAW_COMMAND_CAPACITY)
        return;

    S_DEBUG_DRAW_COMMAND cmd;
    cmd.type = DebugDrawCommandType::Line;
    cmd.flags = 0;
    cmd.colorRGBA8 = pack_clamp_u8(uint4(float4(color, 1.0) * 255.0));

    cmd.p0 = float4(pos0 - float3(size, 0, 0), 1.0);
    cmd.p1 = float4(pos0 + float3(size, 0, 0), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex] = cmd;

    cmd.p0 = float4(pos0 - float3(0, size, 0), 1.0);
    cmd.p1 = float4(pos0 + float3(0, size, 0), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex + 1] = cmd;

    cmd.p0 = float4(pos0 - float3(0, 0, size), 1.0);
    cmd.p1 = float4(pos0 + float3(0, 0, size), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex + 2] = cmd;
}

void DrawDebugCross(float3 pos0, float size, float3 color)
{
    if (g_GlobalUniforms.debugDrawEnabled == 0)
        return;

    uint commandIndex;
    InterlockedAdd(g_GlobalUniforms.DebugDrawBuffer->commandCount, 4, commandIndex);

    if (commandIndex + 4 >= DEBUG_DRAW_COMMAND_CAPACITY)
        return;

    S_DEBUG_DRAW_COMMAND cmd;
    cmd.type = DebugDrawCommandType::Line;
    cmd.flags = 0;
    cmd.colorRGBA8 = pack_clamp_u8(uint4(float4(color, 1.0) * 255.0));

    cmd.p0 = float4(pos0 - float3(size, size, size), 1.0);
    cmd.p1 = float4(pos0 + float3(size, size, size), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex] = cmd;

    cmd.p0 = float4(pos0 - float3(-size, size, size), 1.0);
    cmd.p1 = float4(pos0 + float3(-size, size, size), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex + 1] = cmd;

    cmd.p0 = float4(pos0 - float3(size, -size, size), 1.0);
    cmd.p1 = float4(pos0 + float3(size, -size, size), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex + 2] = cmd;

    cmd.p0 = float4(pos0 - float3(-size, -size, size), 1.0);
    cmd.p1 = float4(pos0 + float3(-size, -size, size), 1.0);
    g_GlobalUniforms.DebugDrawBuffer->commands[commandIndex + 3] = cmd;
}

#endif // !__cplusplus

#endif // _SHADERS_COMMON_H_

