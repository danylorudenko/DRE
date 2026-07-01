#ifndef _GLOBAL_UNIFORM_H_
#define _GLOBAL_UNIFORM_H_

#include "common/shaders_defines.slang"
#include "common/lighting/lights.slang"
#include "common/instances.slang"
#include "common/global_illumination/ddgi_common.slang"
#include "common/debug_draw.slang"

struct GlobalUniforms
{
    // uniform buffer layout
    float4 viewportSize_deltaMS_timeS;
    float4 CameraPos_GenericScalar;
    float3 CameraDir;
    uint   FrameNumber;
    float4 Jitter;

    float4x4 ViewM;
    float4x4 iViewM;
    float4x4 ProjM;
    float4x4 iProjM;
    float4x4 ViewProjM;
    float4x4 iViewProjM;

    float4x4 PrevViewM;
    float4x4 PreviViewM;
    float4x4 PrevProjM;
    float4x4 PreviProjM;
    float4x4 PrevProjJittM;
    float4x4 PreviProjJittM;
    float4x4 PrevViewProjM;
    float4x4 PreviViewProjM;

    float4 SunLightDir;

    uint blueNoiseTextureID;
    uint whiteNoiseTextureID;
    uint debugDrawEnabled;
    uint pad2;

    uint4               lightsCount;

    S_LIGHT*            LightBuffer;
    S_INSTANCE*         InstanceBuffer;

    uint64_t            GlobalGeometryBuffer;
    DebugDrawBuffer*    DebugDrawBuffer;

    DDGIConstantBuffer  ddgiCB;
};

#ifndef __cplusplus
[[vk::binding(0, 2)]] ConstantBuffer<GlobalUniforms> g_GlobalUniforms;

// Global uniform values
float2      GetViewportSize() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.xy; }
float       GetDeltaTime() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.z; }
float       GetTimeS() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.w; }
float3      GetCameraPos() { return g_GlobalUniforms.CameraPos_GenericScalar.xyz; }
float       GetGenericScalar() { return g_GlobalUniforms.CameraPos_GenericScalar.w; }
float3      GetCameraDir() { return g_GlobalUniforms.CameraDir.xyz; }
uint        GetFrameNumber() { return g_GlobalUniforms.FrameNumber; }
float2      GetJitter() { return g_GlobalUniforms.Jitter.xy; }

float4x4    GetCameraViewM() { return g_GlobalUniforms.ViewM; }
float4x4    GetCameraInvViewM() { return g_GlobalUniforms.iViewM; }
float4x4    GetCameraProjM() { return g_GlobalUniforms.ProjM; }
float4x4    GetCameraInvProjM() { return g_GlobalUniforms.iProjM; }
float4x4    GetCameraViewProjM() { return g_GlobalUniforms.ViewProjM; }
float4x4    GetCameraInvViewProjM() { return g_GlobalUniforms.iViewProjM; }

float4x4    GetPrevCameraViewM() { return g_GlobalUniforms.PrevViewM; }
float4x4    GetPrevCameraInvViewM() { return g_GlobalUniforms.PreviViewM; }
float4x4    GetPrevCameraProjM() { return g_GlobalUniforms.PrevProjM; }
float4x4    GetPrevCameraInvProjM() { return g_GlobalUniforms.PreviProjM; }
float4x4    GetPrevCameraViewProjM() { return g_GlobalUniforms.PrevViewProjM; }
float4x4    GetPrevCameraInvViewProjM() { return g_GlobalUniforms.PreviViewProjM; }

float3      GetSunLightDir() { return g_GlobalUniforms.SunLightDir.xyz; }

uint        GetBlueNoiseTextureId() { return g_GlobalUniforms.blueNoiseTextureID; }
uint        GetWhiteNoiseTextureId() { return g_GlobalUniforms.whiteNoiseTextureID; }

uint        GetLightsCount() { return g_GlobalUniforms.lightsCount.x; }
S_LIGHT*    GetLight(uint i) { return g_GlobalUniforms.LightBuffer + i; }

uint        GetInstanceID() { return globalPushConstant.value_int1; }
S_INSTANCE* GetInstance(uint i) { return g_GlobalUniforms.InstanceBuffer + i; }
S_INSTANCE* GetInstance() { return GetInstance(GetInstanceID()); }

uint64_t    GetGlobalGeometry(uint64_t offset) { return g_GlobalUniforms.GlobalGeometryBuffer + offset; }

DDGIConstantBuffer GetDDGI() { return g_GlobalUniforms.ddgiCB; }

#endif // !__cplusplus



#endif // _GLOBAL_UNIFORM_H_
