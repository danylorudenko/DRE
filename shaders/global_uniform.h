#include "shaders_defines.h"
#include "lights.h"

#ifdef __cplusplus
struct GlobalUniforms
#else
layout(set = 2, binding = 0, std140) readonly uniform GlobalUniforms
#endif
{
    // uniform buffer layout
    vec4 viewportSize_deltaMS_timeS;
    vec4 main_CameraPos_GenericScalar;
    vec4 main_CameraDir;
	vec4 main_Jitter;
	
    mat4 main_ViewM;
    mat4 main_iViewM;
    mat4 main_ProjM;
    mat4 main_iProjM;
    mat4 main_ViewProjM;
    mat4 main_iViewProjM;
	
    mat4 main_PrevViewM;
    mat4 main_PreviViewM;
    mat4 main_PrevProjM;
    mat4 main_PreviProjM;
    mat4 main_PrevProjJittM;
    mat4 main_PreviProjJittM;
    mat4 main_PrevViewProjM;
    mat4 main_PreviViewProjM;
	
    vec4 main_LightDir;
    vec4 main_LightRadiance;

    S_LIGHT_GPURef LightBuffer;
    // end
#ifdef __cplusplus
};

#undef vec4
#undef mat4
#else
} g_GlobalUniforms;

// Global uniform values
vec2    GetViewportSize() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.xy; }
float   GetDeltaTime() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.z; }
float   GetTimeS() { return g_GlobalUniforms.viewportSize_deltaMS_timeS.w; }
vec3    GetCameraPos() { return g_GlobalUniforms.main_CameraPos_GenericScalar.xyz; }
float   GetGenericScalar() { return g_GlobalUniforms.main_CameraPos_GenericScalar.w; }
vec3    GetCameraDir() { return g_GlobalUniforms.main_CameraDir.xyz; }
vec2    GetJitter() { return g_GlobalUniforms.main_Jitter.xy; }

mat4    GetCameraViewM() { return g_GlobalUniforms.main_ViewM; }
mat4    GetCameraiViewM() { return g_GlobalUniforms.main_iViewM; }
mat4    GetCameraProjM() { return g_GlobalUniforms.main_ProjM; }
mat4    GetCameraiProjM() { return g_GlobalUniforms.main_iProjM; }
mat4	GetCameraViewProjM() { return g_GlobalUniforms.main_ViewProjM; }
mat4	GetCameraiViewProjM() { return g_GlobalUniforms.main_iViewProjM; }

mat4    GetPrevCameraViewM() { return g_GlobalUniforms.main_PrevViewM; }
mat4    GetPrevCameraiViewM() { return g_GlobalUniforms.main_PreviViewM; }
mat4    GetPrevCameraProjM() { return g_GlobalUniforms.main_PrevProjM; }
mat4    GetPrevCameraiProjM() { return g_GlobalUniforms.main_PreviProjM; }
mat4	GetPrevCameraViewProjM() { return g_GlobalUniforms.main_PrevViewProjM; }
mat4	GetPrevCameraiViewProjM() { return g_GlobalUniforms.main_PreviViewProjM; }

vec3    GetMainLightDir() { return g_GlobalUniforms.main_LightDir.xyz; }
vec3    GetMainLightRadiance() { return g_GlobalUniforms.main_LightRadiance.xyz; }

S_LIGHT_GPURef GetLight(int i) { return g_GlobalUniforms.LightBuffer[i]; }

#endif

