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

#ifndef __cplusplus
layout(set = 3, binding = 2) uniform texture2D causticMap;
#endif

//////////////
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
#endif

#endif // __FORWARD_H__