#ifdef __cplusplus
#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

#define vec4 glm::vec4
#define mat4 glm::mat4
#endif


#ifdef __cplusplus
struct GlobalUniforms
#else
layout(set = 2, binding = 0, std140) readonly uniform GlobalUniforms
#endif
{
    // uniform buffer layout
    vec4 viewportSize_deltaMS_0;
    vec4 main_CameraPos;
    vec4 main_CameraDir;
    mat4 main_ViewM;
    mat4 main_iViewM;
    mat4 main_ProjM;
    mat4 main_iProjM;
    vec4 main_LightDir;
    // end
#ifdef __cplusplus
};
#else
} g_GlobalUniforms;

// Global uniform values
vec2   GetViewportSize() { return g_GlobalUniforms.viewportSize_deltaMS_0.xy; }
float  GetDeltaTime() { return g_GlobalUniforms.viewportSize_deltaMS_0.z; }
vec3   GetCameraPos() { return g_GlobalUniforms.main_CameraPos.xyz; }
vec3   GetCameraDir() { return g_GlobalUniforms.main_CameraDir.xyz; }
mat4   GetCameraViewM() { return g_GlobalUniforms.main_ViewM; }
mat4   GetCameraiViewM() { return g_GlobalUniforms.main_iViewM; }
mat4   GetCameraProjM() { return g_GlobalUniforms.main_ProjM; }
mat4   GetCameraiProj() { return g_GlobalUniforms.main_iProjM; }

vec3   GetMainLightDir() { return g_GlobalUniforms.main_LightDir.xyz; }
#endif

