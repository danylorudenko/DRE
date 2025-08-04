#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "common/shaders_defines.h"
#include "common/global_uniform.h"

//////////////
#ifndef __cplusplus
uint GetInstanceID() { return globalPushConstant.value_int1; }
S_INSTANCE_GPURef GetInstance() { return GetInstance(GetInstanceID()); }
#endif

//////////////
#ifndef __cplusplus
layout(set = 3, binding = 0) uniform texture2D shadowMap;
#endif

//////////////
BEGIN_CONSTANT_BUFFER(ForwardUniform, passUniform, 3, 1)
{
    mat4  shadow_VP;
    vec4  shadow_size;
}
END_CONSTANT_BUFFER(ForwardUniform, passUniform, 3, 1)

//////////////
#ifndef __cplusplus
layout(set = 3, binding = 2) uniform texture2D causticMap;
#endif

///////////////////////////////////////
// ====================================
///////////////////////////////////////

/////////////
// Object ID
#ifndef __cplusplus
vec4 GlobalID2Color()
{
    S_INSTANCE_GPURef Instance = GetInstance();

    float r = (GetGlobalID(Instance) & 0xFF000000) >> 24;
    float g = (GetGlobalID(Instance) & 0x00FF0000) >> 16;
    float b = (GetGlobalID(Instance) & 0x0000FF00) >> 8;
    float a = (GetGlobalID(Instance) & 0x000000FF) >> 0;
    return vec4(r,g,b,a) / 255.0;
}
#endif // __cplusplus

/////////////
// Output
#ifdef DRE_FRAGMENT_SHADER
layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;
layout(location = 2) out vec4 id;
#endif // DRE_FRAGMENT_SHADER
#define FORWARD_PASS_OUTPUT_COUNT 3 // DON'T FORGET

/////////////
// Reusable outputs
#ifdef DRE_FRAGMENT_SHADER
void OutputForwardPass(in S_LIGHTING_RESULT Result, in S_SURFACE Surface)
{
    finalColor = vec4(Result.finalRadiance, 1.0);

    vec4 prev_ndc = GetPrevCameraViewProjM() * Surface.prevWpos;
    prev_ndc /= prev_ndc.w;

    vec2 pixel_pos_uv = gl_FragCoord.xy / GetViewportSize();
    vec2 pixel_pos_ndc = pixel_pos_uv * 2.0 - 1.0;

    vec2 vel = (pixel_pos_ndc - prev_ndc.xy);
    vec2 vel_uv = vel * 0.5;

    velocity = vec2(vel_uv);
    id = GlobalID2Color();
}
#endif // DRE_FRAGMENT_SHADER

#endif // __FORWARD_H__