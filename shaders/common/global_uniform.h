#ifndef _GLOBAL_UNIFORM_H_
#define _GLOBAL_UNIFORM_H_

#include "common/shaders_defines.h"
#include "common/lighting/lights.h"
#include "common/instances.h"

struct GlobalUniforms
{
    // uniform buffer layout
    float4 viewportSize_deltaMS_timeS;
    float4 main_CameraPos_GenericScalar;
    float4 main_CameraDir;
    float4 main_Jitter;

    float4x4 main_ViewM;
    float4x4 main_iViewM;
    float4x4 main_ProjM;
    float4x4 main_iProjM;
    float4x4 main_ViewProjM;
    float4x4 main_iViewProjM;

    float4x4 main_PrevViewM;
    float4x4 main_PreviViewM;
    float4x4 main_PrevProjM;
    float4x4 main_PreviProjM;
    float4x4 main_PrevProjJittM;
    float4x4 main_PreviProjJittM;
    float4x4 main_PrevViewProjM;
    float4x4 main_PreviViewProjM;

    float4 main_SunLightDir;

    float4x4 main_ShadowVP;
    float4 main_ShadowSize;

    // wtf?
    uint4 TEX_ID_shadow;

    uint4               lightsCount;
    S_LIGHT*            LightBuffer;

    S_INSTANCE*         InstanceBuffer;
};

#ifndef __cplusplus
[[vk::binding(0, 2)]] ConstantBuffer<GlobalUniforms> g_GlobalUniforms;

// Global uniform values
float2    GetViewportSize() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.xy; }
float     GetDeltaTime() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.z; }
float     GetTimeS() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.w; }
float3    GetCameraPos() { return g_GlobalUniforms.main_CameraPos_GenericScalar.xyz; }
float     GetGenericScalar() { return g_GlobalUniforms.main_CameraPos_GenericScalar.w; }
float3    GetCameraDir() { return g_GlobalUniforms.main_CameraDir.xyz; }
float2    GetJitter() { return g_GlobalUniforms.main_Jitter.xy; }

float4x4  GetCameraViewM() { return g_GlobalUniforms.main_ViewM; }
float4x4  GetCameraInvViewM() { return g_GlobalUniforms.main_iViewM; }
float4x4  GetCameraProjM() { return g_GlobalUniforms.main_ProjM; }
float4x4  GetCameraInvProjM() { return g_GlobalUniforms.main_iProjM; }
float4x4  GetCameraViewProjM() { return g_GlobalUniforms.main_ViewProjM; }
float4x4  GetCameraInvViewProjM() { return g_GlobalUniforms.main_iViewProjM; }

float4x4  GetPrevCameraViewM() { return g_GlobalUniforms.main_PrevViewM; }
float4x4  GetPrevCameraInvViewM() { return g_GlobalUniforms.main_PreviViewM; }
float4x4  GetPrevCameraProjM() { return g_GlobalUniforms.main_PrevProjM; }
float4x4  GetPrevCameraInvProjM() { return g_GlobalUniforms.main_PreviProjM; }
float4x4  GetPrevCameraViewProjM() { return g_GlobalUniforms.main_PrevViewProjM; }
float4x4  GetPrevCameraInvViewProjM() { return g_GlobalUniforms.main_PreviViewProjM; }

float3    GetSunLightDir() { return g_GlobalUniforms.main_SunLightDir.xyz; }

float4x4  GetSunShadowVP() { return g_GlobalUniforms.main_ShadowVP; }
float2    GetSunShadowSize() { return g_GlobalUniforms.main_ShadowSize.xy; }

uint      GetShadowMapID() { return g_GlobalUniforms.TEX_ID_shadow.x; }

uint        GetLightsCount() { return g_GlobalUniforms.lightsCount.x; }
S_LIGHT*    GetLight(uint i) { return g_GlobalUniforms.LightBuffer + i; }

uint        GetInstanceID() { return globalPushConstant.value_int1; }
S_INSTANCE* GetInstance(uint i) { return g_GlobalUniforms.InstanceBuffer + i; }
S_INSTANCE* GetInstance() { return GetInstance(GetInstanceID()); }

#endif // !__cplusplus



#endif // _GLOBAL_UNIFORM_H_
