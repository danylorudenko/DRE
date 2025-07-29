#ifndef __FORWARD_OUTPUT_H__
#define __FORWARD_OUTPUT_H__

#ifndef __cplusplus
// pass outputs
layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec2 velocity;
layout(location = 2) out vec4 id;
#endif // __cplusplus
#define FORWARD_PASS_OUTPUT_COUNT 3 // DON'T FORGET


#ifndef __cplusplus
/////////////
vec4 GlobalID2Color()
{
    S_INSTANCE_GPURef Instance = GetInstance();

    float r = (GetGlobalID(Instance) & 0xFF000000) >> 24;
    float g = (GetGlobalID(Instance) & 0x00FF0000) >> 16;
    float b = (GetGlobalID(Instance) & 0x0000FF00) >> 8;
    float a = (GetGlobalID(Instance) & 0x000000FF) >> 0;
    return vec4(r,g,b,a) / 255.0;
}

/////////////
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
#endif // __cplusplus

#endif // __FORWARD_OUTPUT_H__